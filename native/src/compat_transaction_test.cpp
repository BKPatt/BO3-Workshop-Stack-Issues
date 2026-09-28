#include "compat_transaction.hpp"
#include <iostream>
#include <stdexcept>

using namespace native_compat;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
constexpr std::array<CompatSite,4> sites{{{0,1},{8,2},{16,3},{24,1}}};
struct Memory {
    std::array<unsigned char,32> bytes{};
    int writes{},fail_write{-1},unreadable{-1};
    bool fail_after_copy{};
    Memory(){for(const auto& s:sites)memcpy(bytes.data()+s.rva,signatures[s.kind-1],length(s.kind));}
    void prepatch(size_t i){const auto& s=sites[i];memcpy(bytes.data()+s.rva,replacements[s.kind-1],length(s.kind));}
    bool read(unsigned rva,void* out,size_t n){if(static_cast<int>(rva)==unreadable)return false;memcpy(out,bytes.data()+rva,n);return true;}
    bool write(unsigned rva,const unsigned char* in,size_t n){const bool fail=writes++==fail_write;if(!fail || fail_after_copy)memcpy(bytes.data()+rva,in,n);return !fail;}
};
int main(){
    for(unsigned mask=0;mask<16;mask++) {
        Memory memory;for(size_t i=0;i<4;i++)if(mask&(1u<<i))memory.prepatch(i);
        const auto before=memory.bytes;
        Transaction<4> tx;
        auto read=[&](auto r,auto p,auto n){return memory.read(r,p,n);};
        auto write=[&](auto r,auto p,auto n){return memory.write(r,p,n);};
        check(tx.apply(sites,read,write),"Original or recognized T7 state refused");
        check(tx.applied+tx.reused==4,"Patch accounting lost a site");
        check(memory.writes==static_cast<int>(tx.applied),"Preexisting patch was rewritten");
        for(size_t i=0;i<4;i++)check(Transaction<4>::inspect(sites[i],read)==SiteState::compatible,"Not fully patched");
        check(tx.rollback(sites,read,write),"Rollback failed");
        check(memory.bytes==before,"Rollback changed a preexisting T7 patch");
        // Simulate a write failing before or after copying bytes at every point.
        for(int fail=0;fail<static_cast<int>(tx.applied);fail++)for(bool after:{false,true}) {
            Memory failing;failing.bytes=before;failing.fail_write=fail;failing.fail_after_copy=after;
            auto rd=[&](auto r,auto p,auto n){return failing.read(r,p,n);};
            auto wr=[&](auto r,auto p,auto n){return failing.write(r,p,n);};
            Transaction<4> attempt;
            check(!attempt.apply(sites,rd,wr),"Injected write failure ignored");
            check(attempt.rollback_ok && failing.bytes==before,"Partial failure did not restore exactly the prior state");
        }
    }
    for(bool unreadable:{false,true}) {
        Memory memory;memory.prepatch(0);
        if(unreadable)memory.unreadable=24;else memory.bytes[24]=0xcc;
        const auto before=memory.bytes;Transaction<4> tx;
        auto read=[&](auto r,auto p,auto n){return memory.read(r,p,n);};
        auto write=[&](auto r,auto p,auto n){return memory.write(r,p,n);};
        check(!tx.apply(sites,read,write),"Unknown/unreadable state accepted");
        check(memory.writes==0 && memory.bytes==before && tx.failed_rva==24,"Validation was not atomic");
    }
    {
        Memory memory;Transaction<4> tx;
        auto read=[&](auto r,auto p,auto n){return memory.read(r,p,n);};
        auto write=[&](auto r,auto p,auto n){return memory.write(r,p,n);};
        check(tx.apply(sites,read,write),"Initial apply failed");
        check(!tx.apply(sites,read,write),"Reapply discarded ownership");
        memory.bytes[8]=0xcc;
        check(!tx.rollback(sites,read,write) && memory.bytes[8]==0xcc,"Rollback overwrote a later unknown patch");
        check(memory.bytes[0]==signatures[0][0],"Rollback did not restore unaffected owned sites");
    }
    std::cout<<"All 16 mixed T7 states, partial-write failures, ownership rollback, and unknown-byte refusal passed.\n";
}
