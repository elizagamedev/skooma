#include "hook.h"

#include "win32.h"
#include <windows.h>

namespace hook
{
    void write_rel_jump(ULONG_PTR src, ULONG_PTR dst)
    {
        DWORD old_flags;
        if (!VirtualProtect(reinterpret_cast<LPVOID>(src), 5, PAGE_READWRITE,
                            &old_flags)) {
            throw win32::get_last_error_exception();
        }
        *reinterpret_cast<uint8_t *>(src) = 0xE9;
        *reinterpret_cast<uint32_t *>(src + 1) = dst - src - 1 - 4;
        VirtualProtect(reinterpret_cast<LPVOID>(src), 5, old_flags, &old_flags);
    }
}
