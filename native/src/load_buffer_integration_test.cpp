// Exercise the actual production callbacks in a separate executable. DllMain
// is not an EXE entry point; no BO3 process or game address is accessed here.
#include "extension.cpp"
#include <iostream>

void check_buffer(bool value,const char* message){if(!value)throw std::runtime_error(message);}
__declspec(noinline) int invoke_prepare(PrepareZones fn,ZoneInfo* in,int count,char* names,ZoneInfo* out,int capacity) {
    const auto result=fn(in,count,names,out,capacity);
    volatile int prevent_tail_call=result;return prevent_tail_call;
}
int capture_prepare(ZoneInfo*,int,char*,ZoneInfo*,int) {
    base=reinterpret_cast<uintptr_t>(_ReturnAddress())-0x1423a0c;return 0;
}
__declspec(noinline) void invoke_schedule(ScheduleZones fn,ZoneInfo* out,int count) {
    fn(out,count);volatile int prevent_tail_call=count;(void)prevent_tail_call;
}
void capture_schedule(ZoneInfo*,int) {base=reinterpret_cast<uintptr_t>(_ReturnAddress())-0x1423a18;}
int convert_fixture(ZoneInfo*,int count,char* names,ZoneInfo* out,int capacity) {
    check_buffer(capacity>=count*2,"Converter did not receive enough temporary records");
    for(int i=0;i<count*2;i++) {
        auto name=names+i*64;
        snprintf(name,64,"fixture_package_%d",i);
        out[i]=ZoneInfo{name,0x40,0,0,0,nullptr,0};
    }
    return count*2;
}
int scheduled{};
void schedule_fixture(ZoneInfo* zones,int count) {
    scheduled=count;
    for(int i=0;i<count;i++)check_buffer(zones[i].name &&
        std::string(zones[i].name)=="fixture_package_"+std::to_string(i),"Scheduled package pointer/order was corrupted");
}
std::array<asset_capacity::Pool,55> fixture_pools;
void* allocate_fixture(int type) {
    auto& pool=fixture_pools.at(type);
    const auto item=pool.free_head;
    if(item){pool.free_head=*static_cast<void**>(item);++pool.allocated;}
    return item;
}
void check_actual_allocator() {
    allocate_asset=allocate_fixture;
    base=reinterpret_cast<uintptr_t>(fixture_pools.data())-0x93883f0;
    // Use the production callback with real Windows memory-headroom reporting,
    // while keeping pool descriptors and allocation/free operations isolated.
    alignas(void*) std::array<unsigned char,48> original{};
    for(const int type:{47,54}) {
        original.fill(0x5a);
        auto& pool=fixture_pools[type];
        pool=asset_capacity::Pool{original.data(),24,2,0,2,nullptr};
        const auto first=allocate_hook(type);
        check_buffer(first && pool.capacity==130 && pool.allocated==3,"Actual allocator callback failed to grow supported exhausted pool");
        check_buffer(pool.original==original.data() && original[0]==0x5a,"Actual callback moved existing headers");
        const auto committed=asset_storage.committed;
        for(int n=1;n<128;n++)check_buffer(allocate_hook(type)!=nullptr,"Expanded pool exhausted early");
        check_buffer(!pool.free_head && pool.allocated==130,"Actual callback failed to consume every new header");
        check_buffer(asset_storage.committed==committed,"Actual callback grew a non-exhausted pool");
        check_buffer(allocate_hook(type) && pool.capacity==258,"Actual callback failed a second growth");
        const auto retained=asset_storage.committed;
        *static_cast<void**>(first)=pool.free_head;pool.free_head=first;--pool.allocated;
        check_buffer(allocate_hook(type)==first && asset_storage.committed==retained,"Actual callback did not reuse a freed extension header");
    }
    check_buffer(allocate_hook(9)==nullptr,"Actual callback grew an unsupported asset pool");
}
int main() {
    prepare_zones=convert_fixture;schedule_zones=schedule_fixture;
    std::array<ZoneInfo,32> stock;
    std::array<char,32*64> names;
    std::array<ZoneInfo,32> input{};
    for(const int count:{1,8,16,17,24,32}) {
        memset(stock.data(),0x5a,sizeof(stock));names.fill('Z');
        invoke_prepare(capture_prepare,input.data(),count,names.data(),stock.data(),32);
        const auto converted=invoke_prepare(prepare_hook,input.data(),count,names.data(),stock.data(),32);
        check_buffer(converted==count*2,"Actual prepare callback lost converted records");
        for(auto byte:std::span(reinterpret_cast<unsigned char*>(stock.data()),sizeof(stock)))
            check_buffer(byte==0x5a,"Actual prepare callback wrote the stock 32-record buffer");
        check_buffer(std::all_of(names.begin(),names.end(),[](char c){return c=='Z';}),"Stock name buffer was overwritten");
        check_buffer(prepared_zones.entries.size()>=static_cast<size_t>(count*4),"Buffers did not grow with demand");
        invoke_schedule(capture_schedule,stock.data(),converted);
        invoke_schedule(schedule_hook,stock.data(),converted);
        check_buffer(scheduled==converted && !prepared_zones.stock_output,"Actual scheduler failed to consume the owned buffer");
    }
    // A different engine call site must retain its original output buffers.
    base=0;
    const auto result=invoke_prepare(prepare_hook,input.data(),4,names.data(),stock.data(),32);
    check_buffer(result==8 && stock[0].name==names.data(),"Unknown call site was redirected");
    check_actual_allocator();
    std::cout<<"Actual prepare/schedule callbacks passed 34, 48 and 64 records without touching stock 32-record buffers; actual allocation callback grew and reused raw-file and script headers; unsupported pools remained unchanged.\n";
}
