#pragma once
#include "stack_logic.hpp"
#include <span>

namespace selection_editor {
// Validate the entire request before replacing anything. Missing providers,
// duplicate IDs and malformed input must leave the applied stack untouched.
template<class T, class Key, class Valid>
std::optional<std::vector<T>> resolve(std::string_view request, std::span<const T> installed, Key key, Valid valid) {
    std::vector<T> result;
    if(request.empty())return result;
    while(!request.empty()) {
        const auto end=request.find(',');
        const auto id=request.substr(0,end);
        if(!stack_logic::valid_id(id))return std::nullopt;
        const auto found=std::find_if(installed.begin(),installed.end(),[&](const auto& item){return valid(item) && key(item)==id;});
        if(found==installed.end() || !stack_logic::select(result,*found,key))return std::nullopt;
        if(end==request.npos)break;
        request.remove_prefix(end+1);
        if(request.empty())return std::nullopt;
    }
    return result;
}

inline std::string lua_string(std::string_view value) {
    std::string result="\"";
    for(const unsigned char c:value) {
        // Byte escapes preserve UTF-8 and prevent names becoming Lua source.
        if(c<32 || c=='"' || c=='\\' || c>=127) {
            result+='\\';
            result+=static_cast<char>('0'+c/100);
            result+=static_cast<char>('0'+c/10%10);
            result+=static_cast<char>('0'+c%10);
        } else result+=static_cast<char>(c);
    }
    return result+'"';
}
}
