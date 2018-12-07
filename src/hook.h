#pragma once

#include <windows.h>

namespace hook
{
    void write_rel_jump(ULONG_PTR src, ULONG_PTR dst);
}
