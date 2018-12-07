#include "prochook.h"

#include "IDirect3DDevice9Vtbl.h"
#include "win32.h"
#include <cstring>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <future>


static HINSTANCE d3d9_instance = NULL;
static IDirect3DDevice9Vtbl d3d9_device_vtbl;
static const uint32_t START_GAME_ADDR = 0x0040ED25;
static const uint32_t START_GAME_ADDR_RET = START_GAME_ADDR + 5;
static const uint32_t TDT_ADDR = 0x0056F7C0;


static void write_rel_jump(uint32_t src, uint32_t dst)
{
    DWORD old_flags;
    if (!VirtualProtect(reinterpret_cast<LPVOID>(src),
                        5, PAGE_READWRITE, &old_flags)) {
        throw win32::get_last_error_exception();
    }
    *reinterpret_cast<uint8_t*>(src) = 0xE9;
    *reinterpret_cast<uint32_t*>(src + 1) = dst - src - 1 - 4;
    VirtualProtect(reinterpret_cast<LPVOID>(src), 5, old_flags, &old_flags);
}

static IDirect3DDevice9Vtbl get_d3d_device_vtbl(HINSTANCE dll_instance)
{
    // Create d3d9 instance
    typedef IDirect3D9 *(WINAPI *Direct3DCreate9_t)(UINT);
    Direct3DCreate9_t _Direct3DCreate9 = reinterpret_cast<Direct3DCreate9_t>(
        GetProcAddress(d3d9_instance, "Direct3DCreate9"));
    IDirect3D9 *d3d9 = _Direct3DCreate9(D3D_SDK_VERSION);
    if (d3d9 == nullptr) {
        throw std::runtime_error("Direct3DCreate9 call failed");
    }

    // Create window
    HWND window = CreateWindowExW(
        WS_EX_LEFT,
        L"STATIC", L"",
        0, 0, 0, 0, 0,
        NULL, NULL,
        dll_instance,
        nullptr);
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
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_NULLREF,
        window,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &params,
        &device);
    if (create_result != D3D_OK) {
        d3d9->Release();
        DestroyWindow(window);
        std::stringstream sstream;
        sstream << "CreateDevice returned "
                << std::hex
                << static_cast<unsigned long>(create_result);
        throw std::runtime_error(sstream.str());
    }
    // We got it! Save a copy
    IDirect3DDevice9Vtbl vtbl;
    std::memcpy(&vtbl,
                *reinterpret_cast<IDirect3DDevice9Vtbl**>(device),
                sizeof(vtbl));
    device->Release();
    d3d9->Release();
    DestroyWindow(window);
    return vtbl;
}

static __declspec(noinline) void IDirect3D9Device9_Present_Hook()
{
    // Regulate FPS
    static const auto spf =
        std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(
            std::chrono::seconds(1)) / 60;
    static std::chrono::time_point<std::chrono::high_resolution_clock> last_tick;
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
        call IDirect3D9Device9_Present_Hook;
        pop ecx;
        push ebp;
        mov ebp, esp;
        mov edi, d3d9_device_vtbl.Present;
        add edi, 5;
        jmp edi;
    }
}

void __declspec(naked) start_game_hook()
{
    __asm {
        // Call tdt
        pushad;
        call TDT_ADDR;
        popad;
        // Replace instructions we overwrote
        add esp, 4;
        xor edx, edx;
        // Jump back
        jmp START_GAME_ADDR_RET;
    }
}

namespace prochook {
    void install(HINSTANCE dll_instance)
    {
        // Load d3d9.dll, create a d3d9 instance, and finally a d3d9 device,
        // all in the name of hooking the vtable functions
        d3d9_instance = LoadLibraryW(L"d3d9.dll");
        if (d3d9_instance == NULL) {
            throw win32::get_last_error_exception();
        }

        // Retrieve the vtable
        d3d9_device_vtbl = get_d3d_device_vtbl(dll_instance);


        // std::async(
        //     []() {
        //         typedef void (*ToggleFn)();
        //         reinterpret_cast<ToggleFn>(0x0056F7C0)();
        //     });

        // Hook functions
        win32::with_suspend_threads(
            []() {
                // Write JMP at game start to run tdt!
                write_rel_jump(START_GAME_ADDR,
                               reinterpret_cast<uint32_t>(start_game_hook));
                // Write JMP to our FPS hook!
                write_rel_jump(reinterpret_cast<uint32_t>(d3d9_device_vtbl.Present),
                               reinterpret_cast<uint32_t>(IDirect3D9Device9_Present));
            });
    }

    void uninstall()
    {
        FreeLibrary(d3d9_instance);
    }
}
