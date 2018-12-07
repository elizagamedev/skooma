#include "Config.h"

#define DIRECTINPUT_VERSION 0x0800
#include "win32.h"
#include <cwchar>
#include <dinput.h>
#include <string>

Config global_config;

static bool read_bool(const WCHAR *section,
                      const WCHAR *name,
                      bool def,
                      const std::wstring &filename)
{
    const WCHAR *defstr = def ? L"true" : L"false";
    WCHAR buffer[256];
    DWORD size = sizeof(buffer) / sizeof(WCHAR);
    GetPrivateProfileStringW(section, name, defstr, buffer, size,
                             filename.c_str());
    if (std::wcsncmp(buffer, L"false", size) == 0) {
        return false;
    }
    if (std::wcsncmp(buffer, L"0", size) == 0) {
        return false;
    }
    if (std::wcsncmp(buffer, L"", size) == 0) {
        return false;
    }
    return true;
}

static int read_int(const WCHAR *section,
                    const WCHAR *name,
                    int def,
                    const std::wstring &filename)
{
    WCHAR buffer[256];
    DWORD size = sizeof(buffer) / sizeof(WCHAR);
    GetPrivateProfileStringW(section, name, std::to_wstring(def).c_str(),
                             buffer, size, filename.c_str());
    return std::stoi(buffer);
}

Config::Config()
    : auto_tdt(true)
    , limit_framerate(true)
    , scrollclick(true)
    , scrollclick_modifier_key(DIK_LSHIFT)
    , scrollclick_disable_scroll(true)
{
    std::wstring filename = win32::get_module_file_name(GetModuleHandleW(NULL));
    std::size_t pos = filename.rfind(L"\\");
    if (pos == std::string::npos) {
        return;
    }
    filename.replace(pos + 1, std::string::npos, L"skooma.ini");

    // Read from ini
    auto_tdt = read_bool(L"auto_tdt", L"enabled", true, filename);
    limit_framerate = read_bool(L"limit_framerate", L"enabled", true, filename);
    scrollclick = read_bool(L"scrollclick", L"enabled", true, filename);
    scrollclick_modifier_key
        = read_int(L"scrollclick", L"modifier_key", DIK_LSHIFT, filename);
    scrollclick_disable_scroll
        = read_bool(L"scrollclick", L"disable_scroll", true, filename);
}
