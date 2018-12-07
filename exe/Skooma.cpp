#include "Skooma.h"

#include "NotificationIcon.h"
#include "win32.h"
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>


static std::condition_variable global_scroll_cv;
static std::mutex global_scroll_mutex;
static bool global_scroll_active = false;

static LRESULT CALLBACK mouse_hook_func(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (wParam == WM_MOUSEWHEEL && (GetKeyState(VK_SHIFT) & 0x8000) != 0) {
        {
            std::unique_lock<std::mutex> lock(global_scroll_mutex);
            global_scroll_active = true;
        }
        global_scroll_cv.notify_one();
        return 1;
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

Skooma::Skooma(HINSTANCE hInstance)
    : instance_(hInstance)
{
}

Skooma::~Skooma()
{
}

void Skooma::run()
{
    NotificationIcon icon(
        instance_, L"SKOOMA");
    extern WindowsHook create_injection_hook();
    WindowsHook injection_hook(create_injection_hook());
    WindowsHook mouse_hook(WH_MOUSE_LL, mouse_hook_func, instance_, 0);

    // This thread will constantly send a click and unclick event pair while
    // scrolling.
    std::thread scrollclick_thread(
        []() {
            for (;;) {
                {
                    // Wait until there are pending scroll events
                    std::unique_lock<std::mutex> lock(global_scroll_mutex);
                    while (!global_scroll_active)
                        global_scroll_cv.wait(lock);
                    global_scroll_active = false;
                }
                // Handle a single click
                INPUT input = {0};
                input.type = INPUT_MOUSE;
                input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                SendInput(1, &input, sizeof(input));
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                ZeroMemory(&input, sizeof(input));
                input.type = INPUT_MOUSE;
                input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                SendInput(1, &input, sizeof(input));
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        });

    win32::run_event_loop();
}
