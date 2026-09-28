#include "asset_capacity.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <cstring>
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main() {
    using namespace asset_capacity;
    constexpr uint64_t gib=1024ull*1024*1024;
    alignas(void*) std::array<unsigned char,48> original{};
    original.fill(0x5a);
    Pool pool{original.data(),24,2,0,2,nullptr};
    const auto before=original;
    Storage storage;
    check(!supported(9,264) && !supported(47,32) && supported(47,24) && supported(54,24),"Indexed/unknown pool enabled");
    check(!growth_bytes(pool,100,8*gib,16*gib) && !growth_bytes(pool,8*gib,100,16*gib),"Low-memory headroom ignored");
    const auto bytes=growth_bytes(pool,8*gib,8*gib,16*gib);
    check(bytes==128*24,"Small-pool growth policy changed");
    check(!storage.grow(pool,bytes,[](size_t)->void*{return nullptr;}) && !pool.free_head && pool.capacity==2 && !storage.committed,"Allocation failure mutated the pool");
    check(storage.grow(pool,bytes),"First growth failed");
    check(pool.original==original.data() && original==before && pool.allocated==2 && pool.capacity==130,"Growth moved or overwrote live assets");
    check(!growth_bytes(pool,8*gib,8*gib,16*gib) && !storage.grow(pool,bytes),"Non-exhausted pool grew again");
    std::vector<void*> live;
    for(int i=0;i<128;i++) {
        auto item=pool.free_head;check(item!=nullptr,"Free list truncated");
        pool.free_head=*static_cast<void**>(item);++pool.allocated;
        std::memset(item,0x31,24);live.push_back(item);
    }
    check(!pool.free_head && pool.allocated==pool.capacity,"Free-list termination or counts incorrect");
    check(storage.grow(pool,growth_bytes(pool,8*gib,8*gib,16*gib)),"Repeated demand did not grow");
    for(auto item:live)check(static_cast<unsigned char*>(item)[0]==0x31,"Later growth damaged live headers");
    // Model the inspected engine's free-list push/pop without moving headers.
    auto released=live.back();*static_cast<void**>(released)=pool.free_head;pool.free_head=released;--pool.allocated;
    check(pool.free_head==released,"Released extension header not reusable");
    pool.free_head=*static_cast<void**>(released);++pool.allocated;
    Pool singleton{original.data(),24,2,1,2,nullptr};
    check(!growth_bytes(singleton,8*gib,8*gib,16*gib) && !storage.grow(singleton,bytes),"Singleton changed");
    Pool invalid{original.data(),24,2,0,3,nullptr};
    check(!growth_bytes(invalid,8*gib,8*gib,16*gib) && !storage.grow(invalid,bytes),"Corrupt usage count accepted");
    Pool full{original.data(),24,INT32_MAX-16,0,INT32_MAX-16,nullptr};
    check(!growth_bytes(full,8*gib,8*gib,16*gib) && !storage.grow(full,bytes),"Capacity arithmetic overflowed");
    check(storage.committed==2*bytes && original==before,"Committed bytes or original contents changed");
    std::cout<<"Demand-only pool growth, stable live pointers, repeated exhaustion/reuse, memory headroom, allocation failure, and overflow guards passed.\n";
}
