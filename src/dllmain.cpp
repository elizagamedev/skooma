#include <windows.h>

#include "dinput8hook.h"
#include "gamehook.h"
#include "win32.h"
#include <atomic>
#include <exception>
#include <future>
#include <memory>

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL,
                               DWORD fdwReason,
                               LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        try {
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
