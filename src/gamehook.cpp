#include "gamehook.h"

#include "hook.h"
#include <chrono>
#include <windows.h>

static const ULONG_PTR START_GAME_ADDR = 0x0040ED25;
static const ULONG_PTR START_GAME_ADDR_RET = START_GAME_ADDR + 5;
static const ULONG_PTR MAIN_LOOP_ADDR = 0x0040EDBD;
static const ULONG_PTR MAIN_LOOP_ADDR_RET = 0x0040EDC3;
static const ULONG_PTR TDT_ADDR = 0x0056F7C0;

static __declspec(naked) void start_game_detour()
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

static __declspec(noinline) void main_loop_hook()
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

static __declspec(naked) void main_loop_detour()
{
    __asm {
        pushad;
        call main_loop_hook;
        popad;
        mov eax, [eax + 0x280];
        jmp MAIN_LOOP_ADDR_RET;
    }
}

namespace gamehook
{
    void install()
    {
        hook::write_rel_jump(START_GAME_ADDR,
                             reinterpret_cast<ULONG_PTR>(start_game_detour));
        hook::write_rel_jump(MAIN_LOOP_ADDR,
                             reinterpret_cast<ULONG_PTR>(main_loop_detour));
    }
}
