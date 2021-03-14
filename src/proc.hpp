#pragma once

#include <Windows.h>
#include <process.h>

// window and process stuff

namespace gd {
    namespace window_shit {
        struct handle_data {
            unsigned long process_id;
            HWND window_handle;
        };

        BOOL is_main_window(HWND handle) {   
            return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
        }

        BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam) {
            handle_data& data = *(handle_data*)lParam;
            unsigned long process_id = 0;
            GetWindowThreadProcessId(handle, &process_id);
            if (data.process_id != process_id || !is_main_window(handle))
                return TRUE;
            data.window_handle = handle;
            return FALSE;   
        }
    }

    unsigned long getGDPID() {
        return _getpid();
    }

    HWND getGDWindow() {
        unsigned long process_id = getGDPID();

        window_shit::handle_data data;
        data.process_id = process_id;
        data.window_handle = 0;

        EnumWindows(window_shit::enum_windows_callback, (LPARAM)&data);

        return data.window_handle;
    }
}

