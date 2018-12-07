#include "Skooma.h"
#include <exception>
#include <windows.h>


int main_cpp(HINSTANCE hInstance,
             HINSTANCE hPrevInstance,
             LPSTR lpCmdLine,
             int nCmdShow)
{
    Skooma skooma(hInstance);
    skooma.run();
    return 0;
}

extern "C" {
    int CALLBACK WinMain(HINSTANCE hInstance,
                         HINSTANCE hPrevInstance,
                         LPSTR lpCmdLine,
                         int nCmdShow)
    {
        try {
            return main_cpp(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
        } catch (const std::exception &e) {
            MessageBoxA(NULL, e.what(), "Skooma", MB_OK | MB_ICONERROR);
            return 1;
        }
    }

#ifndef _WINMAIN_
    int main()
    {
        return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
    }
#endif
}
