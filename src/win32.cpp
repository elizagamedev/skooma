#include "win32.h"

#include <exception>
#include <list>
#include <shlwapi.h>
#include <tlhelp32.h>

namespace win32
{
    std::string get_last_error_string()
    {
        LPVOID lpMsgBuf;
        DWORD dw = GetLastError();

        DWORD size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf, 0, NULL);

        std::string result(static_cast<char *>(lpMsgBuf), size);
        LocalFree(lpMsgBuf);
        return result;
    }

    std::runtime_error get_last_error_exception()
    {
        return std::runtime_error(get_last_error_string());
    }

    std::vector<std::wstring> get_args()
    {
        int argc;
        LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        std::vector<std::wstring> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            args.push_back(std::wstring(argv[i]));
        }
        LocalFree(argv);
        return args;
    }

    std::wstring get_module_file_name(HMODULE module)
    {
        WCHAR *buffer;
        DWORD size = 16;
        for (;;) {
            buffer = new WCHAR[size];
            size = GetModuleFileNameW(module, buffer, size);
            DWORD error = GetLastError();
            if (error == ERROR_SUCCESS) {
                break;
            }
            delete[] buffer;
            if (error == ERROR_INSUFFICIENT_BUFFER) {
                size *= 2;
            } else {
                // Weird error that can't be handled
                throw get_last_error_exception();
            }
        }
        std::wstring result(buffer, size);
        delete[] buffer;
        return result;
    }

    std::vector<uint8_t> get_file_version_info(const std::wstring &file_name)
    {
        DWORD dummy;
        DWORD size = GetFileVersionInfoSizeW(file_name.c_str(), &dummy);
        if (size == 0) {
            throw win32::get_last_error_exception();
        }
        std::vector<uint8_t> result(size);
        if (!GetFileVersionInfoW(file_name.c_str(), NULL, size,
                                 result.data())) {
            throw win32::get_last_error_exception();
        }
        return result;
    }

    void run_event_loop()
    {
        for (;;) {
            MSG msg;
            BOOL bRet = GetMessageW(&msg, NULL, 0, 0);

            if (bRet > 0) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } else if (bRet < 0) {
                throw win32::get_last_error_exception();
            } else {
                break;
            }
        }
    }

    void with_suspend_threads(std::function<void()> func)
    {
        DWORD proc_id = GetCurrentProcessId();
        DWORD thread_id = GetCurrentThreadId();
        std::list<DWORD> suspended_thread_ids;

        // First, iterate through the list, collecting all threads that should
        // be suspended and recording them.
        HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, proc_id);
        if (h == INVALID_HANDLE_VALUE) {
            throw win32::get_last_error_exception();
        }
        THREADENTRY32 te;
        te.dwSize = sizeof(te);
        DWORD min_size = FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID)
                         + sizeof(te.th32OwnerProcessID);
        if (Thread32First(h, &te)) {
            do {
                if (te.dwSize < min_size) {
                    continue;
                }
                if (te.th32OwnerProcessID != proc_id) {
                    continue;
                }
                if (te.th32ThreadID == thread_id) {
                    continue;
                }
                HANDLE thread
                    = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
                if (thread != NULL) {
                    SuspendThread(thread);
                    CloseHandle(thread);
                    suspended_thread_ids.push_back(te.th32ThreadID);
                }
                te.dwSize = sizeof(te);
            } while (Thread32Next(h, &te));
        }
        CloseHandle(h);

        // Now run the function
        std::exception_ptr eptr;
        try {
            func();
        } catch (...) {
            eptr = std::current_exception();
        }

        // Resume the threads
        for (auto thread_id : suspended_thread_ids) {
            HANDLE thread = OpenThread(THREAD_ALL_ACCESS, FALSE, thread_id);
            if (thread != NULL) {
                ResumeThread(thread);
                CloseHandle(thread);
            }
        }

        if (eptr) {
            std::rethrow_exception(eptr);
        }
    }

    std::wstring get_system_directory()
    {
        UINT size = GetSystemDirectoryW(nullptr, 0);
        if (size == 0) {
            throw win32::get_last_error_exception();
        }
        WCHAR *buf = new WCHAR[size];
        if (GetSystemDirectoryW(buf, size) != size - 1) {
            throw win32::get_last_error_exception();
        }
        std::wstring result(buf, size - 1);
        delete[] buf;
        return result;
    }
}
