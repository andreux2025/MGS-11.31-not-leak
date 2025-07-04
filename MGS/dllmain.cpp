#include "minhook/MinHook.h"
#include "Utils.h"
#include "Hooking.h"
#include <thread>

#pragma comment(lib, "minhook/minhook.lib")

using namespace std;
using namespace SDK;

DWORD InitThread(LPVOID)
{
    AllocConsole();
    FILE* fptr;
    freopen_s(&fptr, "CONOUT$", "w+", stdout);

    MH_Initialize();
    Log("Initializing MGS");

    new std::thread(InitImGui);
   
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, InitThread, 0, 0, 0);
        break;
    default:
        break;
    }
    return TRUE;
}