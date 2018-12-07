#include <windows.h>

#include "WindowsHook.h"
#include "win32.h"
#include "prochook.h"
#include <exception>
#include <memory>
#include <atomic>
#include <future>


#pragma data_seg(push, ".shared")
bool first_attach = false;
#pragma data_seg(pop)
#pragma comment(linker, "/SECTION:.shared,RWS")

static HINSTANCE dll_instance;
static bool must_install = false;

extern "C" {
    BOOL WINAPI DllMain(HINSTANCE hinstDLL,
                        DWORD fdwReason,
                        LPVOID lpvReserved)
    {
        dll_instance = hinstDLL;
        switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(dll_instance);

            if (!first_attach) {
                // This is attached to the exe, so return success
                first_attach = true;
                return TRUE;
            }

            // Check if this is Oblivion.exe and hook if so
            try {
                if (win32::get_module_file_name(GetModuleHandle(NULL)) != L"oblivion.exe") {
                    return FALSE;
                }
            } catch (const std::exception &) {
                return FALSE;
            }
            must_install = true;
            return TRUE;
        case DLL_PROCESS_DETACH:
            prochook::uninstall();
            return TRUE;
        }
        return FALSE;
    }
}

static LRESULT CALLBACK injection_hook_func(int nCode, WPARAM wParam, LPARAM lParam)
{
    // The initialization of the D3D device fails if called in DllMain. Do it
    // here instead.
    if (must_install) {
        must_install = false;
        std::async(
            []() {
                try {
                    prochook::install(dll_instance);
                } catch (const std::exception &e) {
                    MessageBoxA(NULL, e.what(), "Skooma", MB_OK | MB_ICONERROR);
                }
            });
    }
    return CallNextHookEx(0, nCode, wParam, lParam);
}

WindowsHook create_injection_hook()
{
    return WindowsHook(WH_CBT, injection_hook_func, dll_instance, 0);
}
