#pragma once

#include <windows.h>
#include <functional>


#define WM_NOTIFYICON (WM_APP+0)
typedef std::function<LRESULT(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)> WindowProc;

class NotificationIcon
{
public:
    NotificationIcon(HINSTANCE hInstance,
                     WCHAR *icon_name,
                     HMENU menu = NULL,
                     WindowProc proc = DefWindowProc);
    ~NotificationIcon();

    void show_menu();
    LRESULT window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
    HINSTANCE instance_;
    WindowProc proc_;
    HMENU menu_;
    HICON icon_;
    HWND window_;
    UINT id_;
};
