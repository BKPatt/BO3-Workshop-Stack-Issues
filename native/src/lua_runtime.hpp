#pragma once
#include "lua_bindings.hpp"
#include "game_build.hpp"
#include <string_view>

// These entry points and layouts were verified on the supported Steam build.
// Call only on the thread currently executing this VM.
namespace lua_runtime {
using Object=lua_bindings::Object;
inline Object*& top(void* vm){return *reinterpret_cast<Object**>(static_cast<char*>(vm)+0x48);}
inline Object*& frame(void* vm){return *reinterpret_cast<Object**>(static_cast<char*>(vm)+0x50);}
inline Object* begin(void* vm){return *reinterpret_cast<Object**>(static_cast<char*>(vm)+0x60);}
inline Object global(void* vm,const char* key){
    return lua_bindings::lookup(reinterpret_cast<Object*>(static_cast<char*>(vm)+0x70)->value,key);
}
inline const char* argument(uintptr_t image,void* vm){
    if(frame(vm)>=top(vm))return nullptr;
    return reinterpret_cast<const char*(*)(void*,Object*,size_t*)>(image+game_build::addresses.lua_to_string)(vm,frame(vm),nullptr);
}
inline bool execute(uintptr_t image,void* vm,std::string_view bytes,const char* name,std::string& error){
    const auto saved_top=top(vm)-begin(vm),saved_frame=frame(vm)-begin(vm);
    const auto global=*reinterpret_cast<char**>(static_cast<char*>(vm)+0x10);
    auto& mode=*reinterpret_cast<int64_t*>(global+0x1d8);
    const auto saved_mode=mode;
    mode=1;
    alignas(8) unsigned char options[32]{};
    auto result=reinterpret_cast<int(*)(void*,const void*,const char*,size_t,const char*)>(image+game_build::addresses.lua_load_buffer)(vm,options,bytes.data(),bytes.size(),name);
    mode=saved_mode;
    if(result==0)result=reinterpret_cast<int(*)(void*,int,int,int)>(image+game_build::addresses.lua_pcall)(vm,0,0,0);
    if(result!=0 && top(vm)>begin(vm)){
        const auto text=reinterpret_cast<const char*(*)(void*,Object*,size_t*)>(image+game_build::addresses.lua_to_string)(vm,top(vm)-1,nullptr);
        error=text?text:"Unknown Lua error";
    }
    top(vm)=begin(vm)+saved_top;frame(vm)=begin(vm)+saved_frame;
    return result==0;
}
}
