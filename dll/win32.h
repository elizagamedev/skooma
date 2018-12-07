#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <exception>
#include <functional>

namespace win32
{
    std::string get_last_error_string();
    std::runtime_error get_last_error_exception();
    std::vector<std::wstring> get_args();
    void run_event_loop();
    std::wstring get_module_file_name(HMODULE module);
    void with_suspend_threads(std::function<void()> func);
}
