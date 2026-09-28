#pragma once
#include <windows.h>
#include <string>
#include <cstdio>
namespace stack_status {
inline constexpr const char* version="1.1.1";
inline std::wstring name(const wchar_t* state,DWORD pid) {
    return std::wstring(L"Local\\BO3WorkshopStack_")+state+L"_"+std::to_wstring(pid);
}
// Independent of BO3/HavokScript layouts, including when the executable is
// unsupported. Readers access the message only after the worker signals done.
struct SharedDetails {
    struct Data { LONG code; char message[2048]; };
    HANDLE mapping{};Data* data{};
    explicit SharedDetails(DWORD pid) {
        mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(Data),name(L"Details",pid).c_str());
        if(mapping)data=static_cast<Data*>(MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(Data)));
    }
    ~SharedDetails(){if(data)UnmapViewOfFile(data);if(mapping)CloseHandle(mapping);}
    void publish(LONG code,const std::string& message) {
        if(!data)return;
        snprintf(data->message,sizeof(data->message),"%s",message.c_str());
        InterlockedExchange(&data->code,code);
    }
    std::string message() const {
        if(!data || !InterlockedCompareExchange(&data->code,0,0))return {};
        return std::string(data->message,strnlen_s(data->message,sizeof(data->message)));
    }
    SharedDetails(const SharedDetails&)=delete;
    SharedDetails& operator=(const SharedDetails&)=delete;
};
struct Signals {
    HANDLE ready{},failed{};
    explicit Signals(DWORD pid) {
        ready=CreateEventW(nullptr,TRUE,FALSE,name(L"Ready",pid).c_str());
        failed=CreateEventW(nullptr,TRUE,FALSE,name(L"Failed",pid).c_str());
    }
    ~Signals(){if(ready)CloseHandle(ready);if(failed)CloseHandle(failed);}
    explicit operator bool() const{return ready && failed;}
    Signals(const Signals&)=delete;
    Signals& operator=(const Signals&)=delete;
};
}
