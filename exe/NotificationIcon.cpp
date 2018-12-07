#include "NotificationIcon.h"

#include "win32.h"
#include <iostream>


static ATOM notify_window_class = NULL;
static UINT num_notify_ids = 0;

static LRESULT CALLBACK window_proc_bootstrap(HWND hwnd,
                                              UINT uMsg,
                                              WPARAM wParam,
                                              LPARAM lParam)
{
    NotificationIcon *ni = reinterpret_cast<NotificationIcon*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (ni != nullptr) {
        return ni->window_proc(hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static ATOM register_notify_window_class(HINSTANCE hInstance)
{
    WNDCLASSW wc = {};
    wc.lpfnWndProc = window_proc_bootstrap;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"NOTIFYICON";
    ATOM cls = RegisterClassW(&wc);
    if (cls == NULL) {
        throw win32::get_last_error_exception();
    }
    return cls;
}

NotificationIcon::NotificationIcon(HINSTANCE hInstance,
                                   WCHAR *icon_name,
                                   HMENU menu,
                                   WindowProc proc)
    : instance_(hInstance)
    , menu_(menu)
    , proc_(std::move(proc))
    , icon_(NULL)
    , window_(NULL)
    , id_(0)
{
    if (notify_window_class == NULL) {
        notify_window_class = register_notify_window_class(hInstance);
    }

    icon_ = static_cast<HICON>(
        LoadImageW(hInstance,
                   icon_name,
                   IMAGE_ICON,
                   64, 64,
                   LR_DEFAULTCOLOR));
    if (icon_ == NULL) {
        throw win32::get_last_error_exception();
    }

    window_ = CreateWindowExW(
        WS_EX_LEFT,
        reinterpret_cast<LPWSTR>(notify_window_class),
        L"", 0, 0, 0, 0, 0,
        NULL, NULL,
        hInstance,
        nullptr);
    if (window_ == NULL) {
        throw win32::get_last_error_exception();
    }

    SetWindowLongPtr(window_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    NOTIFYICONDATAW data = {};
    data.cbSize = sizeof(data);
    data.hWnd = window_;
    data.uID = ++num_notify_ids;
    data.uFlags = NIF_ICON | NIF_MESSAGE;
    data.uCallbackMessage = WM_NOTIFYICON;
    data.hIcon = icon_;
    data.uVersion = NOTIFYICON_VERSION_4;
    if (!Shell_NotifyIconW(NIM_ADD, &data)) {
        throw win32::get_last_error_exception();
    }
    id_ = num_notify_ids;
}

NotificationIcon::~NotificationIcon()
{
    if (id_ > 0) {
        NOTIFYICONDATAW data = {};
        data.cbSize = sizeof(data);
        data.hWnd = window_;
        data.uID = id_;
        Shell_NotifyIconW(NIM_DELETE, &data);
    }
    if (window_ != NULL) {
        DestroyWindow(window_);
    }
    if (icon_ != NULL) {
        DestroyIcon(icon_);
    }
}

NotificationIcon::show_menu()
{
    POINT point;
    GetCursorPos(&point);
    SetForegroundWindow(window_);
    UINT clicked = TrackPopupMenu(
        hMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
        point.x, point.y,
        0, window_, NULL);
}

LRESULT NotificationIcon::window_proc(HWND hwnd,
                                      UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam)
{
    switch (uMsg) {
    case WM_NOTIFYICON:
        switch (lParam) {
        case WM_RBUTTONDOWN:
        case WM_CONTEXTMENU:
            show_menu();
            break;
        }
    }
    return proc_(hwnd, uMsg, wParam, lParam);
}
