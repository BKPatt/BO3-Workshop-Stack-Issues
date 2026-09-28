#pragma once
#include <map>
#include <string>
#include <string_view>
#include "stack_logic.hpp"

namespace asset_priority {
struct Rank {size_t position;std::string provider;};
using Table=std::map<std::string,Rank,std::less<>>;
inline std::string_view base_zone(std::string_view name) {
    // The native converter adds language and patch variants to a mod zone.
    if(name.size()>3 && name[2]=='_')
        for(const auto language:{"bp","ea","en","es","fr","ge","it","ja","po","ru","sc","tc"})
            if(name.substr(0,2)==language){name.remove_prefix(3);break;}
    if(name.ends_with("_patch"))name.remove_suffix(6);
    return name;
}
inline const Rank* find(const Table& table,std::string_view zone) {
    const auto found=table.find(base_zone(zone));
    return found==table.end()?nullptr:&found->second;
}
template<class Mod,class HasZone,class Id>
Table make_table(const std::vector<Mod>& selected,HasZone has_zone,Id id) {
    Table result;
    for(const auto* zone:{"core_mod","zm_mod","mp_mod"}) {
        bool first=true;
        for(size_t i=0;i<selected.size();++i)if(has_zone(selected[i],zone)) {
            const auto provider=std::string(id(selected[i]));
            result.emplace(first?std::string(zone):"mm"+provider+"_"+zone,Rank{i,provider});
            first=false;
        }
    }
    return result;
}
}
