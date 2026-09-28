#pragma once
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace stack_logic {
inline bool valid_id(std::string_view id) {
    return !id.empty() && id.size()<32 && id!="usermaps" &&
        id.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-")==id.npos;
}
inline bool mod_zone(std::string_view name) {
    return name=="core_mod" || name=="zm_mod" || name=="mp_mod";
}
inline bool safe_resource(std::string_view name) {
    if(name.empty() || name.front()=='/' || name.front()=='\\' || name.find(':')!=name.npos)return false;
    size_t start=0;
    for(size_t i=0;i<=name.size();++i)if(i==name.size() || name[i]=='/' || name[i]=='\\') {
        const auto part=name.substr(start,i-start);
        if(part.empty() || part=="." || part=="..")return false;
        start=i+1;
    }
    return true;
}
struct Alias { std::string provider,resource; };
inline std::optional<Alias> parse_alias(std::string_view name,const std::vector<std::string>& providers) {
    for(const auto& id:providers) {
        if(!valid_id(id))continue;
        const auto prefix="mm"+id+"_";
        size_t offset=0;
        if(!name.starts_with(prefix)) {
            // Localized fastfiles use a two-letter prefix, e.g. en_mm<ID>_zm_mod.
            if(name.size()<3 || name[2]!='_' || name[0]<'a' || name[0]>'z' || name[1]<'a' || name[1]>'z' || !name.substr(3).starts_with(prefix))continue;
            offset=3;
        }
        const auto suffix=name.substr(offset+prefix.size());
        if(!mod_zone(suffix))continue;
        return Alias{id,std::string(name.substr(0,offset))+std::string(suffix)};
    }
    return std::nullopt;
}
// BO3 polls the same selection repeatedly during its transition. Selection must
// be idempotent, never a toggle. Every new selection is appended in load order.
template<class T,class Key> bool select(std::vector<T>& values,const T& item,Key key) {
    if(std::any_of(values.begin(),values.end(),[&](const T& m){return key(m)==key(item);}))return false;
    values.push_back(item);
    return true;
}
}
