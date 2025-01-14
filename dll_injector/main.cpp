#include <windows.h>
#include <iostream>
#include <sstream>

#include "dll_injection.h"
#include "process_privilege.h"

#include "params.h"
#include "actions.h"

#define INVALID_PID (-1)



bool action_load(t_params_struct &iParams)
{
    std::cout << "Selected Action: LOAD\n";
    size_t injected = 0;

    if (inject_into_process(iParams.pid, iParams.dll_path.c_str())) {
        if (is_module_in_process(iParams.pid, iParams.dll_path.c_str())) {
            paramkit::print_in_color(MAKE_COLOR(BLACK, LIME), "Injection OK!");
            return true;
        }
    }
    std::cout << "Injection failed\n";
    return false;
}

bool action_check(t_params_struct &iParams)
{
    std::cout << "Selected Action: CHECK\n";
    if (is_module_in_process(iParams.pid, iParams.dll_path.c_str())) {
        paramkit::print_in_color(MAKE_COLOR(BLACK, WHITE), "Module found!");
        return true;
    }
    paramkit::print_in_color(MAKE_COLOR(BLACK, WHITE), "Module not present in the process!");
    return false;
}

bool action_unload(t_params_struct &iParams)
{
    std::cout << "Selected Action: UNLOAD\n";

    bool isFound = false;
    if (!is_module_in_process(iParams.pid, iParams.dll_path.c_str())) {
        std::cout << "Module not present in the process!\n";
        return false;
    }
    if (unload_module(iParams.pid, iParams.dll_path.c_str())) {
        isFound = true;
        paramkit::print_in_color(MAKE_COLOR(BLACK,YELLOW), "Module unloaded!");
    }
    return isFound;
}



int wmain(int argc, const wchar_t * argv[])
{
    t_params_struct iParams = { 0 };
    iParams.pid = INVALID_PID;
    iParams.action = t_actions::ACTION_LOAD;
    {
        InjParams params;
        if (argc < 2) {
            params.printBanner();
            params.printInfo();
            system("pause");
            return 0;
        }
        if (!params.parse(argc, argv)) {
            return 0;
        }
        params.fillStruct(iParams);
    }
    
    if (set_debug_privilege()) {
        std::cout << "[*] Debug privilege set!\n";
    }

    std::string str(iParams.target.begin(), iParams.target.end());
    if (paramkit::is_number(str.c_str())) {
        iParams.pid = paramkit::get_number(str.c_str());
    }

    bool isCreated = false;
    HANDLE hThread = NULL;
    if (iParams.pid == INVALID_PID) {

        PROCESS_INFORMATION pi = { 0 };
        STARTUPINFOW si = { 0 };
        si.cb = sizeof(STARTUPINFOW);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOW;

        BOOL is_ok = CreateProcessW(
            iParams.target.c_str(),
            NULL,
            NULL, NULL, TRUE,
            CREATE_SUSPENDED | CREATE_NEW_CONSOLE,
            NULL, NULL, 
            &si, &pi
        );

        if (!is_ok) {
            std::cerr << "Failed to create the process\n";
            return -1;
        }
        isCreated = true;
        iParams.pid = pi.dwProcessId;
        hThread = pi.hThread;
    }

    std::wcout << "DLL: " << iParams.dll_path << "\n";
    std::cout << "PID: " << iParams.pid << "\n";

    bool res = false;
    switch (iParams.action) {
    case t_actions::ACTION_LOAD:
        res = action_load(iParams); break;
    case t_actions::ACTION_UNLOAD:
        res = action_unload(iParams); break;
    case t_actions::ACTION_CHECK:
        res = action_check(iParams); break;
    }

    std::cout << std::endl;

    if (isCreated) {
        ResumeThread(hThread);
    }
    return res ? 0 : -1;
}
