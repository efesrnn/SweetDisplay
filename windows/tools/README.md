# Encoder discovery

SweetDisplayEncoderProbe uses Media Foundation MFTEnumEx to enumerate registered hardware H.264 encoders, logging UTC, HRESULTs, friendly name and CLSID. It does not capture frames, instantiate an MFT, install anything, or prove encoding works.

Build separately from the WDK sample using the installed CMake/MSVC/SDK:

```powershell
cmake -S . -B out/probe -G 'Visual Studio 18 2026' -A x64
cmake --build out/probe --config Release
./out/probe/Release/SweetDisplayEncoderProbe.exe
```

This target is independent of the unchanged-driver-build gate. A zero count is a valid discovery result, not permission to report hardware encoding or to install codecs automatically.

