#include "d3d9hook.h"

#include "IDirect3DDevice9Vtbl.h"
#include "hook.h"
#include "win32.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <future>
#include <sstream>

static HINSTANCE d3d9_instance = NULL;
static IDirect3DDevice9Vtbl d3d9_device_vtbl;

static IDirect3DDevice9Vtbl get_d3d_device_vtbl(HINSTANCE dll_instance)
{
    // Create d3d9 instance
    typedef IDirect3D9 *(WINAPI * Direct3DCreate9_t)(UINT);
    Direct3DCreate9_t _Direct3DCreate9 = reinterpret_cast<Direct3DCreate9_t>(
        GetProcAddress(d3d9_instance, "Direct3DCreate9"));
    IDirect3D9 *d3d9 = _Direct3DCreate9(D3D_SDK_VERSION);
    if (d3d9 == nullptr) {
        throw std::runtime_error("Direct3DCreate9 call failed");
    }

    // Create window
    HWND window = CreateWindowExW(WS_EX_LEFT, L"STATIC", L"", 0, 0, 0, 0, 0,
                                  NULL, NULL, dll_instance, nullptr);
    if (window == NULL) {
        d3d9->Release();
        // UnregisterClassW(classname, hInstance);
        throw win32::get_last_error_exception();
    }

    // Create d3d9 device
    D3DPRESENT_PARAMETERS params = {};
    params.BackBufferWidth = 1;
    params.BackBufferHeight = 1;
    params.BackBufferCount = 1;
    params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    params.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    params.hDeviceWindow = window;
    params.Windowed = TRUE;
    IDirect3DDevice9 *device;
    HRESULT create_result = d3d9->CreateDevice(
        D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, window,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &params, &device);
    if (create_result != D3D_OK) {
        d3d9->Release();
        DestroyWindow(window);
        std::stringstream sstream;
        sstream << "CreateDevice returned " << std::hex
                << static_cast<unsigned long>(create_result);
        throw std::runtime_error(sstream.str());
    }
    // We got it! Save a copy
    IDirect3DDevice9Vtbl vtbl;
    std::memcpy(&vtbl, *reinterpret_cast<IDirect3DDevice9Vtbl **>(device),
                sizeof(vtbl));
    device->Release();
    d3d9->Release();
    DestroyWindow(window);
    return vtbl;
}

static __declspec(noinline) void IDirect3D9Device9_Present_hook()
{
    // Regulate FPS
    static const auto spf
        = std::chrono::
              duration_cast<std::chrono::high_resolution_clock::duration>(
                  std::chrono::seconds(1))
          / 60;
    static std::chrono::time_point<std::chrono::high_resolution_clock>
        last_tick;
    std::chrono::time_point<std::chrono::high_resolution_clock> now;
    // Spin till we win
    for (;;) {
        now = std::chrono::high_resolution_clock::now();
        if (now - last_tick >= spf) {
            break;
        }
    }
    last_tick = std::move(now);
}

void __declspec(naked) IDirect3D9Device9_Present()
{
    __asm {
        push ecx;
        call IDirect3D9Device9_Present_hook;
        pop ecx;
        push ebp;
        mov ebp, esp;
        mov edi, d3d9_device_vtbl.Present;
        add edi, 5;
        jmp edi;
    }
}

namespace d3d9hook
{
    void install(HINSTANCE dll_instance)
    {
        // Load d3d9.dll, create a d3d9 instance, and finally a d3d9 device,
        // all in the name of hooking the vtable functions
        d3d9_instance = LoadLibraryW(L"d3d9.dll");
        if (d3d9_instance == NULL) {
            throw win32::get_last_error_exception();
        }

        // Check DOS Header
        PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)GetModuleHandleA(NULL);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return FALSE;

        // Check NT Header
        PIMAGE_NT_HEADERS nt
            = MAKE_POINTER(PIMAGE_NT_HEADERS, dos, dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE)
            return FALSE;

        // Check import module table
        PIMAGE_IMPORT_DESCRIPTOR modules = MAKE_POINTER(
            PIMAGE_IMPORT_DESCRIPTOR, dos,
            nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
                .VirtualAddress);
        if (modules == (PIMAGE_IMPORT_DESCRIPTOR)nt)
            return FALSE;

        // Find the correct module
        while (modules->Name) {
            const char *szModule
                = MAKE_POINTER(const char *, dos, modules->Name);

            for (i = 0; i < HOOK_MAX; i++) {
                if (!hooks[i].funcHook && !strcasecmp(hooks[i].name, szModule))
                    break;
            }

            if (i < HOOK_MAX) {
                // Find the correct function
                PIMAGE_THUNK_DATA thunk
                    = MAKE_POINTER(PIMAGE_THUNK_DATA, dos, modules->FirstThunk);
                while (thunk->u1.Function) {
                    for (j = i + 1; j < HOOK_MAX && hooks[j].funcHook; j++) {
                        if (thunk->u1.Function == (DWORD)hooks[j].funcOrig) {
                            // Overwrite
                            DWORD flags;
                            if (!VirtualProtect(&thunk->u1.Function,
                                                sizeof(thunk->u1.Function),
                                                PAGE_READWRITE, &flags)) {
                                return FALSE;
                            }
                            thunk->u1.Function = (DWORD)hooks[j].funcHook;
                            if (!VirtualProtect(&thunk->u1.Function,
                                                sizeof(thunk->u1.Function),
                                                flags, &flags)) {
                                return FALSE;
                            }
                        }
                    }
                    thunk++;
                }
            }
            modules++;
        }
        return TRUE;

        // Retrieve the vtable
        d3d9_device_vtbl = get_d3d_device_vtbl(dll_instance);

        // Write JMP to our FPS hook!
        hook::write_rel_jump(
            reinterpret_cast<uint32_t>(d3d9_device_vtbl.Present),
            reinterpret_cast<uint32_t>(IDirect3D9Device9_Present));
    }

    void uninstall()
    {
        FreeLibrary(d3d9_instance);
    }
}
