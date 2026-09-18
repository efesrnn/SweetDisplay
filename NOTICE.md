# Third-party notices

## Microsoft IndirectDisplay sample

Source: https://github.com/microsoft/Windows-driver-samples
Commit: 67d81f217bc01edf7a4320e4911c11065635acfa
Original path: video/IndirectDisplay
Copyright (c) Microsoft Corporation; upstream license also states Copyright (c) 2015 Microsoft.
License: Microsoft Public License (MS-PL), complete text at
windows/driver/LICENSE-MS-PL.txt.

SweetDisplay derives the driver, device enumerator and project scaffolding from
this sample. Changes include virtual monitor identity, modes, buffer validation,
enumerator lifetime/error handling and local build-output paths. Upstream notices
remain in the copied material. The full upstream checkout is not redistributed.
The .vcxproj scaffolding for the frame probe, Host and Host tests also derives
from that sample. Its MS-PL attribution applies outside windows/driver as described
in LICENSE.md. New frame handoff code includes a bounded GPU pool and metadata
interface; independently authored Host code has no additional license grant yet.

## Xiaomi kernel candidate

Reference only: https://github.com/MiCode/Xiaomi_Kernel_OpenSource
Branch sweet-r-oss; commit 758bb7ef50af360e728662a1ed3b3a1b977a2f13.
No Xiaomi/Linux source tree, downloaded Makefile, firmware or proprietary blob is
included in the public file set. Any future source redistribution must preserve
the licenses and notices applicable to those exact files.

## External tools

Microsoft EWDK/SDK/Visual Studio and Android Platform-Tools are obtained separately
from their official providers and remain subject to their own terms. Their
installation media and binaries are excluded from the public repository.
