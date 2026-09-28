#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <cstring>
#include "compat_transaction.hpp"

// Compatibility for native mod code on the explicitly supported Steam build.
// Steam authentication, networking, files on disk, and game ownership checks
// are untouched. All changes disappear when the game process exits.
namespace native_compat {
inline Transaction<std::size(compat_sites)> transaction;
struct PausedThreads {
    std::vector<HANDLE> handles;
    std::vector<HANDLE> suspended;
    bool pause() {
        const auto snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
        if(snapshot==INVALID_HANDLE_VALUE)return false;
        THREADENTRY32 entry{sizeof(entry)};
        if(!Thread32First(snapshot,&entry)){CloseHandle(snapshot);return false;}
        do {
            if(entry.th32OwnerProcessID!=GetCurrentProcessId() || entry.th32ThreadID==GetCurrentThreadId())continue;
            const auto thread=OpenThread(THREAD_SUSPEND_RESUME|SYNCHRONIZE,FALSE,entry.th32ThreadID);
            if(thread)handles.push_back(thread);
            else if(GetLastError()!=ERROR_INVALID_PARAMETER){CloseHandle(snapshot);return false;}
        } while(Thread32Next(snapshot,&entry));
        CloseHandle(snapshot);suspended.reserve(handles.size());
        for(auto thread:handles) {
            if(SuspendThread(thread)!=DWORD(-1))suspended.push_back(thread);
            else if(WaitForSingleObject(thread,0)!=WAIT_OBJECT_0)return false;
        }
        return true;
    }
    ~PausedThreads(){for(auto thread:suspended)ResumeThread(thread);for(auto thread:handles)CloseHandle(thread);}
};
inline bool write(uintptr_t address,const unsigned char* bytes,size_t size) {
    DWORD previous{},unused{};
    if(!VirtualProtect(reinterpret_cast<void*>(address),size,PAGE_EXECUTE_READWRITE,&previous))return false;
    memcpy(reinterpret_cast<void*>(address),bytes,size);
    const bool restored=VirtualProtect(reinterpret_cast<void*>(address),size,previous,&unused)!=FALSE;
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address),size);
    return restored;
}
inline bool apply(uintptr_t base,std::span<const CompatSite,std::size(compat_sites)> sites=compat_sites) {
    auto read=[&](unsigned rva,void* out,size_t size){SIZE_T count{};return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(base+rva),out,size,&count) && count==size;};
    auto writer=[&](unsigned rva,const unsigned char* bytes,size_t size){return write(base+rva,bytes,size);};
    // Validate while paused, so another startup patch cannot race the check.
    PausedThreads threads;if(!threads.pause())return false;
    return transaction.apply(sites,read,writer);
}
inline bool restore(uintptr_t base,std::span<const CompatSite,std::size(compat_sites)> sites=compat_sites) {
    auto read=[&](unsigned rva,void* out,size_t size){SIZE_T count{};return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(base+rva),out,size,&count) && count==size;};
    auto writer=[&](unsigned rva,const unsigned char* bytes,size_t size){return write(base+rva,bytes,size);};
    PausedThreads threads;if(!threads.pause())return false;
    return transaction.rollback(sites,read,writer);
}
}
