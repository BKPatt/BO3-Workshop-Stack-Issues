#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <vector>

namespace asset_capacity {
struct Pool {
    void* original;
    uint32_t item_size;
    int32_t capacity;
    uint32_t singleton;
    int32_t allocated;
    void* free_head;
};
static_assert(sizeof(Pool)==32);
static_assert(offsetof(Pool,free_head)==24);
// Raw-file and compiled-script headers are addressed by pointer. Image/model/
// weapon pools have additional index-based users and are deliberately excluded.
inline bool supported(int type,uint32_t size) {return (type==47 || type==54) && size==24;}
inline size_t growth_bytes(const Pool& pool,uint64_t available_physical,uint64_t available_commit,uint64_t total_physical) {
    if(pool.singleton || !pool.original || pool.free_head || pool.capacity<=0 ||
       pool.allocated!=pool.capacity || pool.item_size!=24)return 0;
    const uint64_t reserve=std::min<uint64_t>(1024ull*1024*1024,total_physical/8);
    if(available_physical<=reserve || available_commit<=reserve)return 0;
    // Grow only the exhausted pool, by 50% (at least 128 headers). Leave system
    // headroom and consume at most one eighth of the currently available budget.
    const auto room=std::min(available_physical-reserve,available_commit-reserve)/8;
    const auto wanted=std::max<int64_t>(128,pool.capacity/2);
    const auto count=std::min<uint64_t>({static_cast<uint64_t>(wanted),room/pool.item_size,
        static_cast<uint64_t>(std::numeric_limits<int32_t>::max()-pool.capacity)});
    return count>=128?static_cast<size_t>(count*pool.item_size):0;
}
struct Free {void operator()(void* p) const noexcept {std::free(p);}};
class Storage {
    std::vector<std::unique_ptr<void,Free>> blocks;
public:
    size_t committed{};
    template<class Allocate>
    bool grow(Pool& pool,size_t bytes,Allocate allocate) noexcept {
        if(!bytes || bytes%24 || pool.item_size!=24 || pool.singleton || pool.free_head ||
           !pool.original || pool.capacity<=0 || pool.allocated!=pool.capacity ||
           bytes/24>static_cast<size_t>(std::numeric_limits<int32_t>::max()-pool.capacity))return false;
        std::unique_ptr<void,Free> block(allocate(bytes));
        if(!block)return false;
        auto start=static_cast<unsigned char*>(block.get());
        for(size_t offset=0;offset<bytes;offset+=24)
            *reinterpret_cast<void**>(start+offset)=offset+24<bytes?start+offset+24:nullptr;
        try {blocks.push_back(std::move(block));}catch(...){return false;}
        // Keep every original/live address intact. Only add a new free-list
        // segment; the game's allocator/free path continues owning each item.
        pool.capacity+=static_cast<int32_t>(bytes/24);
        pool.free_head=start;
        committed+=bytes;
        return true;
    }
    bool grow(Pool& pool,size_t bytes) noexcept {
        return grow(pool,bytes,[](size_t n){return std::calloc(1,n);});
    }
};
}
