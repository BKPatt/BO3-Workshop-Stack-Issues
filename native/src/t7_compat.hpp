#pragma once
#include <windows.h>
#include <winternl.h>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace t7_compat {
// Scroptss T7 3.04 Windows DLL, SHA-256:
// bc4c9e4edb895f237477a9074552a8186e69b7d36db4886d983dc13c143645c6.
// Protection::ZwContinue is zero-initialized and never assigned, unlike the
// separate global ZwContinue in dllmain.cpp. Its error handler calls this slot.
inline constexpr size_t image_size=0x4e000, slot_rva=0x48bc8;
inline constexpr size_t handler_begin=0xd830, handler_end=0xda88;
inline constexpr uint64_t handler_fingerprint=0xa617cefdaf8f8313ull;
enum class Result { unsupported, repaired, already_valid, occupied };

inline Result initialize_null_slot(void* volatile* slot,void* continuation) noexcept {
    if(!continuation)return Result::unsupported;
    const auto previous=InterlockedCompareExchangePointer(slot,continuation,nullptr);
    return !previous?Result::repaired:(previous==continuation?Result::already_valid:Result::occupied);
}

// Called synchronously by the Windows loader. Only reads mapped image memory
// and uses an interlocked intrinsic: no allocation, logging, OS calls, DLL
// imports, or locks inside this path. Never call into T7 before its DllMain.
inline Result repair_image(void* image,size_t size,void* continuation) noexcept {
    if(!image || size!=image_size || !continuation)return Result::unsupported;
    const auto bytes=static_cast<unsigned char*>(image);
    const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || dos->e_lfanew>0x800)return Result::unsupported;
    const auto pe=reinterpret_cast<const IMAGE_NT_HEADERS64*>(bytes+dos->e_lfanew);
    if(pe->Signature!=IMAGE_NT_SIGNATURE || pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 ||
       pe->FileHeader.TimeDateStamp!=0x6a6b01c7 || pe->FileHeader.NumberOfSections!=6 ||
       pe->FileHeader.SizeOfOptionalHeader!=sizeof(IMAGE_OPTIONAL_HEADER64) ||
       pe->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
       pe->OptionalHeader.SizeOfImage!=image_size || pe->OptionalHeader.AddressOfEntryPoint!=0x17e90)return Result::unsupported;
    // Check the exact writable data section containing the aligned pointer.
    const auto sections=IMAGE_FIRST_SECTION(pe);
    bool writable=false;
    for(unsigned i=0;i<6;i++)if(sections[i].VirtualAddress==0x25000 &&
       sections[i].Misc.VirtualSize==0x241e8 && sections[i].Characteristics==0xc0000040)writable=true;
    if(!writable)return Result::unsupported;
    // This complete 600-byte handler has no base relocations and T7 does not
    // detour it. Its fingerprint binds the call instruction to the slot RVA.
    uint64_t hash=14695981039346656037ull;
    for(size_t i=handler_begin;i<handler_end;i++){hash^=bytes[i];hash*=1099511628211ull;}
    if(hash!=handler_fingerprint)return Result::unsupported;
    return initialize_null_slot(reinterpret_cast<void* volatile*>(bytes+slot_rva),continuation);
}

struct LoadedData {
    ULONG flags;
    const UNICODE_STRING* full_name;
    const UNICODE_STRING* base_name;
    void* base;
    ULONG size;
};
union NotificationData { LoadedData loaded; LoadedData unloaded; };
using Notification=void(CALLBACK*)(ULONG,const NotificationData*,void*);
using Register=NTSTATUS(NTAPI*)(ULONG,Notification,void*,void**);
using Unregister=NTSTATUS(NTAPI*)(void*);

inline bool is_t7(const UNICODE_STRING* name) noexcept {
    constexpr wchar_t expected[]=L"t7patch.dll";
    if(!name || !name->Buffer || name->Length!=sizeof(expected)-sizeof(wchar_t))return false;
    for(size_t i=0;i<sizeof(expected)/sizeof(wchar_t)-1;i++) {
        wchar_t c=name->Buffer[i];if(c>=L'A' && c<=L'Z')c+=L'a'-L'A';
        if(c!=expected[i])return false;
    }
    return true;
}

struct Adapter {
    void* cookie{};
    void* continuation{};
    Unregister unregister_notification{};
    std::atomic<unsigned> repaired{},already_valid{},unsupported{},occupied{};
    void record(Result result) noexcept {
        switch(result) {
        case Result::repaired:repaired.fetch_add(1,std::memory_order_relaxed);break;
        case Result::already_valid:already_valid.fetch_add(1,std::memory_order_relaxed);break;
        case Result::occupied:occupied.fetch_add(1,std::memory_order_relaxed);break;
        default:unsupported.fetch_add(1,std::memory_order_relaxed);break;
        }
    }
    static void CALLBACK notified(ULONG reason,const NotificationData* data,void* context) noexcept {
        if(reason!=1 || !data || !is_t7(data->loaded.base_name))return;
        auto& self=*static_cast<Adapter*>(context);
        self.record(repair_image(data->loaded.base,data->loaded.size,self.continuation));
    }
    bool start() {
        if(cookie)return true;
        const auto ntdll=GetModuleHandleW(L"ntdll.dll");
        const auto register_notification=reinterpret_cast<Register>(GetProcAddress(ntdll,"LdrRegisterDllNotification"));
        unregister_notification=reinterpret_cast<Unregister>(GetProcAddress(ntdll,"LdrUnregisterDllNotification"));
        continuation=reinterpret_cast<void*>(GetProcAddress(ntdll,"ZwContinue"));
        if(!register_notification || !unregister_notification || !continuation)return false;
        if(register_notification(0,notified,this,&cookie)<0){cookie=nullptr;return false;}
        // Registration first closes the gap between inspection and future loads.
        // Hold an ordinary module reference while inspecting a prior T7 load.
        HMODULE existing{};
        if(GetModuleHandleExW(0,L"t7patch.dll",&existing)) {
            const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(existing);
            if(dos->e_magic==IMAGE_DOS_SIGNATURE && dos->e_lfanew>=0 && dos->e_lfanew<=0x800) {
                const auto pe=reinterpret_cast<const IMAGE_NT_HEADERS64*>(reinterpret_cast<unsigned char*>(existing)+dos->e_lfanew);
                record(repair_image(existing,pe->OptionalHeader.SizeOfImage,continuation));
            }
            FreeLibrary(existing);
        }
        return true;
    }
    void stop(){if(cookie){unregister_notification(cookie);cookie=nullptr;}}
    Adapter()=default;
    Adapter(const Adapter&)=delete;
    Adapter& operator=(const Adapter&)=delete;
};
}
