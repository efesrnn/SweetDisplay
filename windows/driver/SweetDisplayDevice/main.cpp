// Derived from Microsoft IddSampleApp; see ../LICENSE-MS-PL.txt and README.md.
#include <cstdio>
#include <windows.h>
#include <swdevice.h>
#include <conio.h>

struct CreationState {
    HANDLE event = nullptr;
    HRESULT result = E_PENDING;
};

void WINAPI CreationCallback(HSWDEVICE, HRESULT result, PVOID context, PCWSTR)
{
    auto* state = static_cast<CreationState*>(context);
    state->result = result;
    SetEvent(state->event);
}

int main()
{
    CreationState state;
    state.event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!state.event) {
        printf("CreateEvent failed: %lu\n", GetLastError());
        return 1;
    }
    HSWDEVICE device = nullptr;
    SW_DEVICE_CREATE_INFO info = {};
    info.cbSize = sizeof(info);
    info.pszInstanceId = L"SweetDisplayDriver";
    info.pszzHardwareIds = L"SweetDisplayDriver\0";
    info.pszzCompatibleIds = L"SweetDisplayDriver\0";
    info.pszDeviceDescription = L"SweetDisplay AMOLED";
    info.CapabilityFlags = SWDeviceCapabilitiesRemovable |
        SWDeviceCapabilitiesSilentInstall | SWDeviceCapabilitiesDriverRequired;

    const HRESULT hr = SwDeviceCreate(L"SweetDisplayDriver", L"HTREE\\ROOT\\0", &info,
        0, nullptr, CreationCallback, &state, &device);
    if (FAILED(hr)) {
        printf("SwDeviceCreate failed: 0x%08lx\n", static_cast<unsigned long>(hr));
        CloseHandle(state.event);
        return 1;
    }
    const DWORD wait = WaitForSingleObject(state.event, 10000);
    if (wait != WAIT_OBJECT_0 || FAILED(state.result)) {
        printf("Device creation failed: wait=%lu, HRESULT=0x%08lx\n", wait,
            static_cast<unsigned long>(wait == WAIT_OBJECT_0 ? state.result : E_PENDING));
        // SwDeviceClose waits for any callback before the state/event is destroyed.
        SwDeviceClose(device);
        CloseHandle(state.event);
        return 1;
    }
    printf("Software device created. This does not prove the display driver started.\n");
    printf("Check Windows Display Settings. Press X to disconnect the virtual device.\n");
    for (;;) {
        const int key = _getch();
        if (key == 'x' || key == 'X') break;
    }
    SwDeviceClose(device);
    CloseHandle(state.event);
    return 0;
}
