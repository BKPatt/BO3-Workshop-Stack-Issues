#pragma once
#include <algorithm>
#include <cstddef>
#include <deque>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "stack_logic.hpp"

namespace stack_logic {
struct ZoneInfo {
    const char* name;
    int alloc_flags, free_flags, alloc_slot, free_slot;
    const void* buffer;
    size_t buffer_size;
};
static_assert(sizeof(ZoneInfo)==0x28);

// Package-name pointers must survive growth of the plan, including short names
// stored inside std::string itself. Deque preserves each string's address.
class ZonePlan {
    std::deque<std::string> names;
    void append(ZoneInfo zone,std::string name) {
        names.push_back(std::move(name));
        zone.name=names.back().c_str();
        zones.push_back(zone);
    }
public:
    std::vector<ZoneInfo> zones;
    template<class Mod,class HasZone,class Id>
    ZonePlan(std::span<const ZoneInfo> requested,const std::vector<Mod>& selected,HasZone has_zone,Id id) {
        zones.reserve(requested.size());
        for(auto original:requested) {
            if(!original.name && original.alloc_flags==0x40 &&
               std::any_of(selected.begin(),selected.end(),[&](const auto& m){return has_zone(m,"core_mod");}))
                original.name="core_mod";
            zones.push_back(original);
            if(!original.name || !mod_zone(original.name))continue;
            bool first=true;
            for(const auto& mod:selected) {
                if(!has_zone(mod,original.name))continue;
                if(first){first=false;continue;}
                auto extra=original;
                extra.free_flags=0;extra.free_slot=0;
                append(extra,"mm"+std::string(id(mod))+"_"+original.name);
            }
        }
        const auto contains=[&](std::string_view name) {
            return std::any_of(zones.begin(),zones.end(),[&](const auto& z){return z.name && name==z.name;});
        };
        const auto restore_mode=[&](const char* common,const char* mod_zone,int flags,int free_flags) {
            if(contains(mod_zone) || !contains(common))return;
            bool first=true;
            for(const auto& mod:selected) {
                if(!has_zone(mod,mod_zone))continue;
                append(ZoneInfo{nullptr,flags,first?free_flags:0,0,0,nullptr,0},first?std::string(mod_zone):"mm"+std::string(id(mod))+"_"+mod_zone);
                first=false;
            }
        };
        restore_mode("core_common","core_mod",0x40,0);
        // BO3 consults only the last selected mod and omits zm_mod when that
        // provider has no Zombies package. Restore the observed native record
        // for earlier providers, including its 0x80 unload mask on the first.
        restore_mode("zm_common","zm_mod",0x100,0x80);
    }
    // A copy would retain pointers into the source plan's name storage.
    ZonePlan(const ZonePlan&)=delete;
    ZonePlan& operator=(const ZonePlan&)=delete;
};
}
