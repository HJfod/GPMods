#include <Windows.h>
#include "src/NPB.hpp"

DWORD WINAPI load_thread(void* hModule) {
    if (!NPB::createHook()) {
        MessageBoxA(nullptr, "NoProgressBar: Unable to create hook!", "Error", MB_ICONERROR);
        
        NPB::unload();
        FreeLibraryAndExitThread((HMODULE)hModule, 0);
        exit(0);

        return 1;
    }

    #ifdef GDCONSOLE
    NPB::awaitUnload();

    NPB::unload();

    FreeLibraryAndExitThread((HMODULE)hModule, 0);

    exit(0);
    #endif

    return 0;
}

DWORD WINAPI unload_thread(void* hModule) {
    NPB::unload();

    FreeLibraryAndExitThread((HMODULE)hModule, 0);

    exit(0);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            CreateThread(0, 0x1000, load_thread, hModule, 0, 0);
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH: break;
        case DLL_PROCESS_DETACH:
            CreateThread(0, 0x1000, unload_thread, hModule, 0, 0);
            break;
    }
    
    return true;
}

