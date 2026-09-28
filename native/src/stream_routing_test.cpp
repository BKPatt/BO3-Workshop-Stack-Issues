#include "extension.cpp"
#include <fstream>
#include <iostream>
#include <thread>

void check_stream(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
const char dep_mod[]="zm_mod",dep_shared[]="zm_levelcommon";
int native_steps{};
bool dependencies(const char* kind,const char* zone,const char** cursor) {
    check_stream(std::string(kind)=="xpak_read" && std::string(zone)=="zm_mod","Alias was not translated to the native dependency group");
    ++native_steps;
    if(!*cursor){*cursor=dep_mod;return true;}
    if(*cursor==dep_mod){*cursor=dep_shared;return true;}
    check_stream(*cursor==dep_shared,"An alias pointer was passed as the native iterator cursor");
    *cursor=nullptr;return false;
}
int native_path(const char*,const char*,int size,char* out,int,const char*){return snprintf(out,size,"stock");}
AssetEntry swapped_bank{};
AssetEntry* link_bank(AssetEntry*,int){return &swapped_bank;}
const char* stream_test_zone(const AssetEntry* entry){return entry->rest[0]==1?"zm_mod":"core_common";}
bool read_sound(bool streamed,void*,void*) {
    char path[512];
    path_hook("snd/all/zm_mod.all",streamed?".sabs":".sabl",sizeof(path),path,1,"2");
    check_stream(std::string(path).find("/1/snd/all/zm_mod.all.")!=std::string::npos,"Background sound read lost the bank's owner");
    return true;
}
int main() {
    stack_initialized=true;
    zone_dependency=dependencies;zone_path=native_path;
    const auto root=std::filesystem::temp_directory_path()/("bo3-stream-test-"+std::to_string(GetCurrentProcessId()));
    std::filesystem::create_directories(root/"1");std::filesystem::create_directories(root/"2");
    struct Cleanup {std::filesystem::path root;~Cleanup(){std::error_code ec;std::filesystem::remove_all(root,ec);}} cleanup{root};
    for(const auto* id:{"1","2"}) {
        std::ofstream(root/id/"zm_mod.xpak")<<"fixture";
        WorkshopData mod{};strcpy_s(mod.publisher_id,id);
        strcpy_s(mod.zone_directory,(root/id).generic_string().c_str());remember_stream_provider(mod);
    }
    const char* first_alias=nullptr;
    for(const auto* id:{"1","2"}) {
        const auto zone="mm"+std::string(id)+"_zm_mod";
        const char* cursor=nullptr;
        check_stream(dependency_hook("xpak_read",zone.c_str(),&cursor),"Missing first dependency");
        check_stream(std::string(cursor)==zone,"Owned XPAK was not uniquely named");
        if(id[0]=='1')first_alias=cursor;else check_stream(cursor!=first_alias,"Providers shared an alias pointer");
        char path[512];
        path_hook(cursor,".xpak",sizeof(path),path,1,"2");
        check_stream(std::filesystem::path(path)==root/id/"zm_mod.xpak","XPAK opened from the wrong provider");
        check_stream(dependency_hook("xpak_read",zone.c_str(),&cursor) && cursor==dep_shared,"Shared dependency was renamed or skipped");
        check_stream(!dependency_hook("xpak_read",zone.c_str(),&cursor) && !cursor,"Dependency iterator did not terminate");
    }
    // Unload enumerates the same aliases even after the menu clears selection.
    stack.clear();const char* cursor=nullptr;
    check_stream(dependency_hook("xpak_read","mm1_zm_mod",&cursor) && cursor==first_alias,"Unload could not find the loaded XPAK identity");
    check_stream(native_steps==7,"Unexpected native iteration count");
    auto parsed=stack_logic::parse_stream_alias("en_mm2_extra_dependency.xpak",{"1","2"});
    check_stream(parsed && parsed->provider=="2" && parsed->resource=="en_extra_dependency.xpak","Localized streaming dependency lost its provider");
    for(const char* bad:{"../mm2_zm_mod.xpak","mm2_../bad.xpak","mm99_zm_mod.xpak","mm2_zm_mod.ff","snd/mm2_zm_mod.xpak"})
        check_stream(!stack_logic::parse_stream_alias(bad,{"1","2"}),"An unrelated/unsafe stream alias was accepted");
    int incoming{},linked{};AssetEntry entry{10,0,&incoming};swapped_bank={10,0,&linked};
    entry.rest[0]=1;swapped_bank.rest[0]=1;asset_zone_name=stream_test_zone;
    priority_table.store(std::make_shared<const asset_priority::Table>(asset_priority::Table{{"zm_mod",{0,"1"}}}));
    link_asset=link_bank;current_extra="1";asset_hook(&entry,0);
    check_stream(sound_owners.at(&incoming)=="1" && sound_owners.at(&linked)=="1","Linked bank ownership was lost when headers changed");
    open_sound_bank=read_sound;void* load=&linked;
    std::exception_ptr failed;
    std::thread worker([&]{try {
        current_extra="2";
        check_stream(sound_bank_hook(true,nullptr,&load),"Streamed sound read failed");
        check_stream(sound_bank_hook(false,nullptr,&load),"Loaded sound read failed");
        check_stream(current_extra=="2","Sound callback did not restore the caller's provider");
    } catch(...){failed=std::current_exception();}});
    worker.join();if(failed)std::rethrow_exception(failed);
    current_extra.clear();entry.rest[0]=2;swapped_bank.rest[0]=2;asset_hook(&entry,0);
    check_stream(!sound_owners.contains(&incoming) && !sound_owners.contains(&linked),"Stock bank reuse retained a stale mod owner");
    std::cout<<"Actual callbacks passed independent XPAKs, cursor round trips, unload identities, shared dependencies, localization, swapped sound headers, background bank reads and TLS restoration.\n";
}
