#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <bcrypt.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "status.hpp"
#include "build_identity.hpp"

// Loads only this project's DLL into the installed BO3 process. No executable patch.
struct Handle {
    HANDLE value{};
    ~Handle(){if(value && value!=INVALID_HANDLE_VALUE) CloseHandle(value);}
};
std::wstring process_path(HANDLE process) {
    std::wstring path(32768,L'\0'); DWORD size=static_cast<DWORD>(path.size());
    if(!QueryFullProcessImageNameW(process,0,path.data(),&size)) return {};
    path.resize(size); return path;
}
int await_ready(const stack_status::Signals& status,HANDLE process) {
    const HANDLE events[]{status.ready,status.failed,process};
    const auto result=WaitForMultipleObjects(3,events,FALSE,15000);
    if(result==WAIT_OBJECT_0){
        std::cout<<"BO3 Workshop Stack is ready.\nOpen MODS in the game and load each mod you want.\n[ON 1], [ON 2], etc. show the stack. U clears all mods.\n";return 0;
    }
    if(result==WAIT_OBJECT_0+1){
        stack_status::SharedDetails details(GetProcessId(process));
        const auto message=details.message();
        std::cerr<<(message.empty()?"The extension refused initialization. See bo3_stack.log, then restart BO3.":message)<<"\n";
    }
    else if(result==WAIT_OBJECT_0+2)std::cerr<<"BO3 exited before the extension was ready.\n";
    else std::cerr<<"Initialization has not reported success. See bo3_stack.log before retrying.\n";
    return 11;
}
int wmain() {
    wchar_t self[32768]; GetModuleFileNameW(nullptr,self,32768);
    const auto dll=std::filesystem::path(self).parent_path()/L"bo3_stack.dll";
    if(!std::filesystem::is_regular_file(dll)){std::cerr<<"bo3_stack.dll is missing.\n";return 1;}
    Handle snapshot{CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0)};
    PROCESSENTRY32W entry{sizeof(entry)}; DWORD pid{};
    if(Process32FirstW(snapshot.value,&entry)) do {
        if(_wcsicmp(entry.szExeFile,L"blackops3.exe")==0){if(pid){std::cerr<<"Close duplicate BO3 instances.\n";return 2;}pid=entry.th32ProcessID;}
    } while(Process32NextW(snapshot.value,&entry));
    if(!pid){std::cerr<<"Start BO3 through Steam and wait for the main menu, then run BO3Stack.\n";return 3;}
    Handle process{OpenProcess(SYNCHRONIZE|PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|PROCESS_CREATE_THREAD,FALSE,pid)};
    if(!process.value){std::cerr<<"Cannot open BO3: "<<GetLastError()<<"\n";return 4;}
    const auto game_path=process_path(process.value);
    const auto observed_hash=build_identity::sha256(game_path);
    if(!game_build::find(observed_hash)) {
        std::cerr<<game_build::unsupported_message(observed_hash)<<"\n";return 5;
    }
    const auto loader=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
    HMODULE owner{};
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(loader),&owner);
    wchar_t owner_path[32768]; GetModuleFileNameW(owner,owner_path,32768);
    const auto owner_name=std::filesystem::path(owner_path).filename().wstring();
    stack_status::Signals status(pid);
    if(!status){std::cerr<<"Cannot create initialization status signals.\n";return 12;}
    Handle modules{CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid)};
    MODULEENTRY32W mod{sizeof(mod)}; uintptr_t remote_owner{};
    if(Module32FirstW(modules.value,&mod)) do {
        if(_wcsicmp(mod.szModule,L"bo3_stack.dll")==0){std::cout<<"Extension is already attached from "<<std::filesystem::path(mod.szExePath)<<"\n";return await_ready(status,process.value);}
        if(_wcsicmp(mod.szModule,owner_name.c_str())==0)remote_owner=reinterpret_cast<uintptr_t>(mod.modBaseAddr);
    } while(Module32NextW(modules.value,&mod));
    if(!remote_owner){std::cerr<<"Cannot resolve Windows DLL loader.\n";return 6;}
    const auto bytes=(dll.native().size()+1)*sizeof(wchar_t);
    const auto remote=VirtualAllocEx(process.value,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    SIZE_T written{};
    if(!remote || !WriteProcessMemory(process.value,remote,dll.c_str(),bytes,&written) || written!=bytes){
        if(remote) VirtualFreeEx(process.value,remote,0,MEM_RELEASE);
        std::cerr<<"Cannot pass extension path to BO3.\n";return 7;
    }
    const auto remote_loader=remote_owner+(reinterpret_cast<uintptr_t>(loader)-reinterpret_cast<uintptr_t>(owner));
    Handle thread{CreateRemoteThread(process.value,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(remote_loader),remote,0,nullptr)};
    if(!thread.value){VirtualFreeEx(process.value,remote,0,MEM_RELEASE);std::cerr<<"DLL load failed.\n";return 8;}
    const auto wait=WaitForSingleObject(thread.value,15000);
    if(wait!=WAIT_OBJECT_0){std::cerr<<"DLL load is pending; inspect the game and log before retrying.\n";return 9;}
    DWORD result{}; GetExitCodeThread(thread.value,&result);
    VirtualFreeEx(process.value,remote,0,MEM_RELEASE);
    if(!result){std::cerr<<"Windows did not load the DLL.\n";return 10;}
    return await_ready(status,process.value);
}
