# DEVICE PHASE 2C-T1 — touch disconnect release reliability

**Status: VERIFIED.** DEVICE PHASE 2C-T remains **PARTIAL** and is not
retroactively relabelled. This bounded follow-up resolves only the Windows
disconnect/release ambiguity recorded by that phase.

## Preserved failure and exact localization

The original `inject-reconnect-1` evidence is unchanged. Its target-side
terminal `UP` remains valid evidence that the tested pointer was released, but
not proof that the old release implementation was generally correct.

The old evidence contains this exact sequence:

| Record | Value | What is proved |
|---|---:|---|
| `INJECT_RETRY` | 1460 (`ERROR_TIMEOUT`) | The old `InjectTouchInput` wrapper captured 1460 immediately after that API returned false. This was an API-call failure, not a controller, target-acknowledgement, worker-wait or teardown deadline. |
| `RELEASE_ERROR` | 87 (`ERROR_INVALID_PARAMETER`) | The old release catch path called `GetLastError` only after a C++ exception had unwound. That value could have been overwritten or stale. The evidence does not prove that a second `InjectTouchInput` call returned 87. |

Thus the timeout source is identified exactly. The invalid-parameter report is
conclusively localized to defective error capture; its underlying API call and
payload cannot be reconstructed from the old evidence and are not invented.
The failed evidence is retained rather than suppressed or reclassified.

The audit also found four state-machine defects which could make teardown
timing-dependent:

- initialization occurred on the Host main thread while injection and teardown
  occurred on the transport worker;
- a stationary contact had no periodic update frame;
- requested coordinates and the last successfully delivered coordinates were
  conflated, so a failed pending move could make a later `UP` use the wrong
  location; and
- the old path cleared local active state after an attempted release even when
  API acknowledgement was uncertain.

## Win32 contract and implemented correction

Microsoft's [`InitializeTouchInjection`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-initializetouchinjection)
and [`InjectTouchInput`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-injecttouchinput)
documentation is the authority. The implementation now observes these material
rules: initialization precedes injection; every frame contains all contacts
that remain active; `DOWN`, `UPDATE` and `UP` use documented pointer-flag
combinations; and an `UP` uses the same location as its immediately preceding
successful `UPDATE`. A teardown sends one all-active `UPDATE` frame and then one
all-active `UP` frame. It does not use `CANCELED` as a substitute for ordinary
release.

The transport worker is now the single logical and physical owner of
configuration, `InitializeTouchInjection`, every `InjectTouchInput` call,
session replacement and final release. Transport close requests release by
calling the endpoint on that same worker before it invalidates the session and
flushes queued video. Host destruction first performs bounded transport finish;
the endpoint destructor is only a final idempotent guard.

The state machine separately retains desired and last-successfully-delivered
contact state plus the injected-contact mask. It coalesces pending moves, emits
a 50 ms active-contact keepalive, and captures `GetLastError` immediately after
each failed API call together with stage, complete payload, flags, coordinates,
contact count and attempt. Only documented transient `ERROR_NOT_READY` receives
at most two 2 ms retries. `ERROR_TIMEOUT` and `ERROR_INVALID_PARAMETER` are not
treated as retryable. A failed release records `RELEASE_UNCERTAIN`, preserves
the active mask and blocks a fresh session instead of silently forgetting
contacts.

The API does not document a general partial-success guarantee for a failed
injection call. The implementation therefore does not infer target state from
an error result; the independent target remains the release oracle and local
state remains uncertain on failure.

## Windows-only gate

`t1-local-4` ran the production `TouchInput::Session` against the independent
SweetDisplay pointer target. Each scenario ran 20 times:

| Scenario | Runs | Unclean final state |
|---|---:|---:|
| DOWN → UP | 20 | 0 |
| DOWN → MOVE → UP | 20 | 0 |
| two DOWN → sequential UP | 20 | 0 |
| two DOWN → simultaneous teardown | 20 | 0 |
| simulated transport disconnect | 20 | 0 |
| session replacement | 20 | 0 |
| Host shutdown | 20 | 0 |
| repeated MOVE/disconnect | 20 | 0 |
| rapid DOWN/disconnect | 20 | 0 |

Across 180 scenario instances, the API ledger has 921 rows, zero failed API
calls and zero retries. The target recorded exactly 220 `DOWN` and 220 `UP`
events. All 180 harness rows ended with active mask zero and `release_clean=1`;
120 teardown paths recorded `RELEASE_ALL` and none recorded
`RELEASE_UNCERTAIN`.

Earlier harness-controller setup errors (`t1-local-1` through `t1-local-3`) are
preserved. They respectively exposed a missing generation predicate, exclusive
target-log sharing and an inactive SweetDisplay target; none was accepted as a
touch-state result.

## Real-phone reconnect gate

The first seven accepted cycles in preserved `t1-phone-2` cover three active
single-contact disconnects, three disconnects during MOVE and one active
two-contact disconnect. Each old session released, each reconnect created a
fresh session, and each fresh session required a new `DOWN`/`UP`. The target's
UP delta was two for each single-contact cycle and three for the two-contact
cycle, including the post-reconnect fresh tap. That run's later controller
snapshot missed a two-contact hold, so its overall result remains `ERROR`; its
451 API-ledger rows nevertheless contain zero API failures/retries and its
target ends at zero active contacts.

`t1-phone-3` continued only cycles 8 and 9. Both active two-contact disconnects
passed, followed by a normal Host shutdown while one contact was active. The
controller deliberately repeated a prompt when the operator lifted before the
controlled cut; such an attempt was not counted as an accepted disconnect.
Accepted cycles each recorded an independent target UP delta of three, a fresh
session and a required fresh tap.

Combined accepted real-device coverage is therefore:

- three active single-contact disconnects;
- three disconnects during contact movement;
- three active two-contact disconnects; and
- one normal Host shutdown with an active contact.

The two retained real runs contain 917 API-ledger rows, zero failed API calls
and zero retries. The independent target reached two simultaneous contacts and
both runs ended with zero active contacts. `t1-phone-3` has Host/pattern exit
code zero, three ready protocol sessions, zero protocol errors, zero final
pending frames and queue peak three. Its bounded transport accounting includes
one explicit overflow, four queue-aborted frames and one unconfirmed frame
around the intentional reconnects; resynchronization recovered through the
normal IDR path. Hardware-only H.264 remained active, software fallback was
false, classification was A-only, and encoder drain/clean shutdown succeeded.

Protocol tests separately reject a correctly formed touch record from a retired
session after a fresh touch handshake. The final deterministic suite passes
58,161 protocol/parser/queue/resync checks.

## Safety, health and evidence boundary

Before and after the accepted continuation, Secure Boot was on, HVCI was on,
the same Windows boot remained active, and the SweetDisplay display/monitor
devices were both OK with problem code zero. Android USB composition was exactly
`mtp,adb` before and after. The controller used only the existing ordinary APK,
authorized ADB process stop/start and ephemeral ADB forwarding.

No Fastboot, root, `su`, `adb root`, remount, AVB/partition/SELinux operation,
ConfigFS/FunctionFS/NCM/HID/UDC/descriptor change, kernel/module build, Windows
driver installation, certificate or Windows security-setting change occurred.
No APK privilege was added. No commit or push was made.

Raw logs, topology, session values, device data, access units and other machine
evidence remain under the ignored `docs/evidence/private` tree. Public documents
contain no serial number, ADB identifier or other unique device identifier.

## Result and hard stop

DEVICE PHASE 2C-T1 is **VERIFIED** for the tested single-owner Windows release
state machine, Windows-only stress matrix, repeated real single/two-contact
disconnects, session replacement and normal Host shutdown. DEVICE PHASE 2C-T
remains **PARTIAL** as its historical outcome. No DEVICE PHASE 2C-1, Windows
PHASE 3E, USB HID or other follow-on phase was started.
