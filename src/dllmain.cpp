#include "dinput8hook.h"
#include "gamehook.h"
#include "win32.h"
#include <atomic>
#include <cwchar>
#include <exception>
#include <future>
#include <memory>
#include <strsafe.h>
#include <windows.h>

static bool is_good_oblivion()
{
    std::vector<uint8_t> version_info = win32::get_file_version_info(
        win32::get_module_file_name(GetModuleHandleW(NULL)));
    UINT str_len;
    WCHAR *str_data;
    if (!VerQueryValueW(version_info.data(),
                        L"\\StringFileInfo\\040904b0\\ProductName",
                        reinterpret_cast<LPVOID *>(&str_data), &str_len)) {
        return false;
    }
    if (wcsncmp(str_data, L"TES4: Oblivion", str_len) != 0) {
        return false;
    }
    if (!VerQueryValueW(version_info.data(),
                        L"\\StringFileInfo\\040904b0\\ProductVersion",
                        reinterpret_cast<LPVOID *>(&str_data), &str_len)) {
        return false;
    }
    if (wcsncmp(str_data, L"1.0.228", str_len) != 0) {
        return false;
    }
    return true;
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL,
                               DWORD fdwReason,
                               LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        try {
            if (!is_good_oblivion()) {
                throw std::runtime_error("Invalid Oblivion version");
            }
            dinput8hook::install();
            gamehook::install();
        } catch (const std::exception &e) {
            MessageBoxA(NULL, e.what(), "Skooma", MB_OK | MB_ICONERROR);
            return FALSE;
        }
        break;
    case DLL_PROCESS_DETACH:
        dinput8hook::uninstall();
        break;
    }
    return TRUE;
}
