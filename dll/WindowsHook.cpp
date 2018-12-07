#include "WindowsHook.h"

#include "win32.h"

WindowsHook::WindowsHook(int idHook,
                         HOOKPROC lpfn,
                         HINSTANCE hmod,
                         DWORD dwThreadId)
{
    if ((hook_ = SetWindowsHookEx(idHook, lpfn, hmod, dwThreadId)) == NULL) {
        throw win32::get_last_error_exception();
    }
}

WindowsHook::WindowsHook(WindowsHook &&o)
{
    using std::swap;
    swap(hook_, o.hook_);
}

WindowsHook &WindowsHook::operator=(WindowsHook &&o)
{
    using std::swap;
    swap(hook_, o.hook_);
    return *this;
}

WindowsHook::~WindowsHook()
{
    if (hook_ != NULL) {
        UnhookWindowsHookEx(hook_);
        hook_ = NULL;
    }
}
