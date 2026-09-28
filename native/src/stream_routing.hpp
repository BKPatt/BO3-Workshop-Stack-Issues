#pragma once
#include "stack_logic.hpp"

namespace stack_logic {
// Stream filenames can include dependencies besides the three mod fastfiles.
// Only our generated basename namespace and known stream extensions are accepted.
inline std::optional<Alias> parse_stream_alias(std::string_view resource,const std::vector<std::string>& providers) {
    if(!safe_resource(resource) || !resource.ends_with(".xpak"))return {};
    const auto slash=resource.find_last_of("/\\");
    if(slash!=resource.npos)return {};
    const auto stem=resource.substr(0,resource.size()-5);
    for(const auto& id:providers) {
        if(!valid_id(id))continue;
        const auto prefix="mm"+id+"_";
        size_t offset=0;
        if(!stem.starts_with(prefix)) {
            if(stem.size()<3 || stem[2]!='_' || stem[0]<'a' || stem[0]>'z' || stem[1]<'a' || stem[1]>'z' || !stem.substr(3).starts_with(prefix))continue;
            offset=3;
        }
        const auto suffix=stem.substr(offset+prefix.size());
        if(suffix.empty())continue;
        return Alias{id,std::string(stem.substr(0,offset))+std::string(suffix)+".xpak"};
    }
    return {};
}
}
