// Read-only hardware MFT discovery. Does not instantiate an encoder or capture a desktop.
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <cwchar>

int wmain()
{
    SYSTEMTIME now{};
    GetSystemTime(&now);
    std::wprintf(L"utc=%04u-%02u-%02uT%02u:%02u:%02uZ probe=hardware-h264-discovery\n",
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::wprintf(L"CoInitializeEx HRESULT=0x%08lX\n", static_cast<unsigned long>(hr));
        return 1;
    }
    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    if (FAILED(hr)) {
        std::wprintf(L"MFStartup HRESULT=0x%08lX\n", static_cast<unsigned long>(hr));
        CoUninitialize();
        return 1;
    }
    IMFActivate** activations = nullptr;
    UINT32 count = 0;
    MFT_REGISTER_TYPE_INFO output{ MFMediaType_Video, MFVideoFormat_H264 };
    hr = MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER,
        MFT_ENUM_FLAG_HARDWARE | MFT_ENUM_FLAG_SORTANDFILTER,
        nullptr, &output, &activations, &count);
    std::wprintf(L"MFTEnumEx HRESULT=0x%08lX hardware_h264_count=%u\n",
        static_cast<unsigned long>(hr), count);
    if (SUCCEEDED(hr)) {
        for (UINT32 i = 0; i < count; ++i) {
            wchar_t* name = nullptr;
            UINT32 length = 0;
            const HRESULT nameHr = activations[i]->GetAllocatedString(
                MFT_FRIENDLY_NAME_Attribute, &name, &length);
            GUID clsid{};
            const HRESULT idHr = activations[i]->GetGUID(MFT_TRANSFORM_CLSID_Attribute, &clsid);
            wchar_t id[40] = L"unknown";
            if (SUCCEEDED(idHr)) { StringFromGUID2(clsid, id, ARRAYSIZE(id)); }
            std::wprintf(L"encoder[%u] name=\"%ls\" clsid=%ls name_hr=0x%08lX clsid_hr=0x%08lX\n",
                i, name ? name : L"unknown", id,
                static_cast<unsigned long>(nameHr), static_cast<unsigned long>(idHr));
            CoTaskMemFree(name);
            activations[i]->Release();
        }
        CoTaskMemFree(activations);
    }
    const HRESULT shutdownHr = MFShutdown();
    CoUninitialize();
    std::wprintf(L"MFShutdown HRESULT=0x%08lX\n", static_cast<unsigned long>(shutdownHr));
    std::wprintf(L"Discovery only: activation, NV12/D3D11 input, encoding, throughput and latency are NOT TESTED.\n");
    return FAILED(hr) || FAILED(shutdownHr) ? 1 : 0;
}
