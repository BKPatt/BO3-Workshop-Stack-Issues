#include "zone_plan.hpp"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
using stack_logic::ZoneInfo;
using stack_logic::ZonePlan;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Mod {std::string id;bool core,zm,mp;};
const auto id=[](const Mod& m){return m.id;};
const auto has=[](const Mod& m,std::string_view name){
    return name=="core_mod"?m.core:name=="zm_mod"?m.zm:name=="mp_mod"?m.mp:false;
};
int main() {
    const int buffer=42;
    const std::array requested{
        ZoneInfo{"core_mod",0x40,0x80,2,3,&buffer,sizeof(buffer)},
        ZoneInfo{"zm_mod",0x100,0x200,4,5,nullptr,0},
        ZoneInfo{"mp_mod",0x400,0x800,6,7,nullptr,0},
        ZoneInfo{"zm_owchallenge",0x1000,0x2000,8,9,&buffer,sizeof(buffer)}
    };
    // Cover the old boundary and growth across deque blocks. Short provider IDs
    // also exercise names that std::string stores inline (dangling if moved).
    for(const int count:{0,1,4,5,6,32,128,512}) {
        std::vector<Mod> mods;
        for(int i=0;i<count;i++)mods.push_back({std::to_string(i+1),true,true,true});
        ZonePlan plan(requested,mods,has,id);
        const size_t per_zone=std::max(count,1);
        check(plan.zones.size()==per_zone*3+1,"Package count omitted or duplicated a provider");
        for(size_t kind=0;kind<3;kind++)for(size_t provider=0;provider<per_zone;provider++) {
            const auto& zone=plan.zones[kind*per_zone+provider];
            const auto& original=requested[kind];
            const std::string expected=provider?"mm"+mods[provider].id+"_"+original.name:original.name;
            check(zone.name && expected==zone.name,"Package name was corrupted or load order changed");
            check(zone.alloc_flags==original.alloc_flags && zone.alloc_slot==original.alloc_slot &&
                  zone.buffer==original.buffer && zone.buffer_size==original.buffer_size,"Package allocation/buffer metadata changed");
            check(zone.free_flags==(provider?0:original.free_flags) && zone.free_slot==(provider?0:original.free_slot),"An extra package inherited unload flags");
        }
        check(plan.zones.back().name==requested.back().name && plan.zones.back().free_flags==0x2000,"Custom map was expanded or altered");
    }
    const std::vector<Mod> sparse{{"empty",false,false,false},{"first",true,false,true},
        {"second",false,true,false},{"third",true,true,true},{"fourth",true,false,false},{"fifth",true,true,false}};
    ZonePlan mixed(requested,sparse,has,id);
    const std::vector<std::string> expected{"core_mod","mmthird_core_mod","mmfourth_core_mod","mmfifth_core_mod",
        "zm_mod","mmthird_zm_mod","mmfifth_zm_mod","mp_mod","mmthird_mp_mod","zm_owchallenge"};
    check(mixed.zones.size()==expected.size(),"Missing per-mode packages changed expansion count");
    for(size_t i=0;i<expected.size();i++)check(expected[i]==mixed.zones[i].name,"First available provider or sparse order is wrong");

    const std::array startup{ZoneInfo{"core_common",1,2,3,4,nullptr,0}};
    ZonePlan injected(startup,sparse,has,id);
    check(injected.zones.size()==5 && std::string(injected.zones[1].name)=="core_mod" &&
          std::string(injected.zones.back().name)=="mmfifth_core_mod","Startup omitted selected core packages");
    for(size_t i=1;i<injected.zones.size();i++)check(injected.zones[i].alloc_flags==0x40 && !injected.zones[i].free_flags && !injected.zones[i].free_slot,"Injected core package metadata is wrong");
    const std::array restore{ZoneInfo{nullptr,0x40,7,2,8,nullptr,0}};
    ZonePlan restored(restore,sparse,has,id);
    check(restored.zones.size()==4 && std::string(restored.zones[0].name)=="core_mod" && restored.zones[0].free_flags==7,"Missing stock core package was not restored");
    const std::vector<Mod> empty;
    ZonePlan untouched(restore,empty,has,id);
    check(untouched.zones.size()==1 && !untouched.zones[0].name,"An empty stack changed an unload request");
    ZonePlan no_requests({},sparse,has,id);
    check(no_requests.zones.empty(),"An empty load request manufactured packages");
    const std::array with_core{startup[0],requested[0]};
    ZonePlan once(with_core,sparse,has,id);
    check(once.zones.size()==5,"Core-common startup duplicated existing core packages");
    const std::array zombies{ZoneInfo{"zm_common",0x80,0,0,0,nullptr,0}};
    const std::vector<Mod> core_last{{"first",false,true,false},{"second",false,true,false},{"last",true,false,false}};
    ZonePlan missing_zombies(zombies,core_last,has,id);
    check(missing_zombies.zones.size()==3 && std::string(missing_zombies.zones[1].name)=="zm_mod" &&
          std::string(missing_zombies.zones[2].name)=="mmsecond_zm_mod","A core-only last selection hid earlier Zombies packages");
    check(missing_zombies.zones[1].alloc_flags==0x100 && missing_zombies.zones[1].free_flags==0x80 &&
          missing_zombies.zones[2].alloc_flags==0x100 && missing_zombies.zones[2].free_flags==0,"Restored Zombies metadata differs from the native load");
    ZonePlan empty_zombies(zombies,empty,has,id);
    check(empty_zombies.zones.size()==1,"Empty selection added a Zombies package");
    const std::array with_zombies{zombies[0],requested[1]};
    ZonePlan zombies_once(with_zombies,core_last,has,id);
    check(zombies_once.zones.size()==3,"Existing Zombies request was duplicated");
    std::cout<<"Package plans through 512 providers / 1,537 packages, stable names, sparse modes, map isolation, unload flags, and core startup passed.\n";
}
