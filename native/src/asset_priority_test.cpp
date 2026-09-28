#include "extension.cpp"
#include <iostream>

void check_priority(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::array<std::string,64> test_zones;
const char* test_zone(const AssetEntry* entry){return test_zones.at(entry->rest[0]).c_str();}
int fallback_calls{};
int native_comparison(const AssetEntry*,const AssetEntry*){++fallback_calls;return 1;}
WorkshopData priority_mod(const char* id){WorkshopData value{};strcpy_s(value.publisher_id,id);return value;}
AssetEntry priority_entry(int type,void* header,unsigned char zone){AssetEntry result{type,0,header,{}};result.rest[0]=zone;return result;}
AssetEntry* priority_link(AssetEntry* incoming,int) {
    static AssetEntry active{};
    // Simulate the verified native swap of header contents and zone metadata.
    if(!active.header){active=*incoming;return &active;}
    if(priority_hook(incoming,&active)) {
        std::swap(*static_cast<int*>(incoming->header),*static_cast<int*>(active.header));
        std::swap(incoming->rest[0],active.rest[0]);
    }
    return &active;
}
int main() {
    try {
        stack_initialized=true;stack={priority_mod("11"),priority_mod("22"),priority_mod("33")};
        const auto has=[](const auto&,const auto*){return true;};
        auto table=asset_priority::make_table(stack,has,[](const auto& m){return m.publisher_id;});
        check_priority(table.size()==9,"Missing zone priorities");
        for(const auto* zone:{"zm_mod","core_mod","mp_mod","en_zm_mod","fr_core_mod_patch"})
            check_priority(asset_priority::find(table,zone)->position==0,"Base/localized/patch priority differs");
        check_priority(asset_priority::find(table,"en_mm22_zm_mod_patch")->position==1,"Localized alias priority differs");
        check_priority(!asset_priority::find(table,"zm_prototype") && !asset_priority::find(table,"mm99_zm_mod"),"Unselected/map zone got mod priority");
        test_zones[1]="core_mod";test_zones[2]="mm22_zm_mod";test_zones[3]="zm_mod";test_zones[4]="zm_prototype";
        asset_zone_name=test_zone;native_asset_priority=native_comparison;
        priority_table.store(std::make_shared<const asset_priority::Table>(table));
        int first_header=11,second_header=22;
        auto first=priority_entry(23,&first_header,1),second=priority_entry(23,&second_header,2);
        check_priority(priority_hook(&first,&second)==1 && priority_hook(&second,&first)==0,"ON 1 did not win both comparison directions across phases");
        second.rest[0]=3;check_priority(priority_hook(&second,&first)==1,"Same-provider native precedence changed");
        second.rest[0]=4;check_priority(priority_hook(&second,&first)==1,"Map/stock precedence changed");
        second.rest[0]=2;second.rest[2]=1;check_priority(priority_hook(&second,&first)==1,"Default priority changed");
        second.rest[2]=0;second.rest[3]=1;check_priority(priority_hook(&second,&first)==1,"Native special priority changed");second.rest[3]=0;
        std::swap(stack[0],stack[1]);
        table=asset_priority::make_table(stack,has,[](const auto& m){return m.publisher_id;});
        test_zones[1]="mm11_core_mod";test_zones[2]="zm_mod";
        priority_table.store(std::make_shared<const asset_priority::Table>(table));
        check_priority(priority_hook(&second,&first)==1 && priority_hook(&first,&second)==0,"Rebuilt stack did not reverse priority");
        stack.clear();priority_table.store(std::make_shared<const asset_priority::Table>());
        check_priority(priority_hook(&first,&second)==1,"Cleared selection retained priorities");
        // A composed list must remain active even when its final header comes
        // from a lower-priority mod. Exact requests remain deduplicated.
        stack={priority_mod("11"),priority_mod("22")};
        table=asset_priority::make_table(stack,has,[](const auto& m){return m.publisher_id;});
        priority_table.store(std::make_shared<const asset_priority::Table>(table));
        test_zones[1]="zm_mod";test_zones[2]="mm22_zm_mod";
        precache::Record request{7,"custom_weapon"};
        auto merged=std::make_shared<precache::Records>(precache::Records{request});retained_precaches.push_back(merged);
        precache::Header original{"zm_mod",&request,1,0},combined{"zm_mod",merged->data(),1,0};
        first=priority_entry(85,&original,1);second=priority_entry(85,&combined,2);
        check_priority(priority_hook(&second,&first)==1 && priority_hook(&first,&second)==0,"Top priority hid a composed precache");
        struct Script{const char* name;uint32_t length,padding;const unsigned char* buffer;};
        auto script=std::make_shared<std::vector<unsigned char>>(128);retained_startup_scripts.push_back(script);
        unsigned char old_script[128]{};Script original_script{"scripts/test.gsc",128,0,old_script},merged_script{"scripts/test.gsc",128,0,script->data()};
        first=priority_entry(54,&original_script,1);second=priority_entry(54,&merged_script,2);
        check_priority(priority_hook(&second,&first)==1 && priority_hook(&first,&second)==0,"Top priority hid a composed startup script");
        // A lower-priority sound bank must not relabel the winning bank's owner.
        first=priority_entry(10,&first_header,1);second=priority_entry(10,&second_header,2);link_asset=priority_link;
        current_extra="11";const auto active=asset_hook(&first,1);
        current_extra="22";check_priority(asset_hook(&second,1)==active,"Native active header changed unexpectedly");
        check_priority(sound_owners.at(active->header)=="11" && sound_owners.at(second.header)=="22","Sound ownership followed the losing incoming provider");
        // Reverse ranking in-place to exercise the native header-content swap.
        auto reversed=table;reversed.at("zm_mod").position=1;reversed.at("mm22_zm_mod").position=0;
        priority_table.store(std::make_shared<const asset_priority::Table>(reversed));
        asset_hook(&second,1);
        check_priority(sound_owners.at(active->header)=="22" && sound_owners.at(second.header)=="11","Sound ownership did not follow swapped zone metadata");
        check_priority(fallback_calls>=15,"Native comparison was bypassed");
        std::cout<<"Production comparator passed top-first precedence, cross-phase order, localization, reorder/clear, native fallbacks, composed assets and winning sound ownership.\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
