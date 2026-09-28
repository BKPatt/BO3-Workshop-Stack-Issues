#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
namespace lua_bindings {
struct Object {uint32_t type,pad;uintptr_t value;};
inline bool boolean_true(Object object) {
    // HKS uses a 32-bit boolean within an otherwise pointer-sized union.
    return (object.type&15)==1 && static_cast<uint32_t>(object.value)!=0;
}
struct Node {Object key,value;};
template<class T> bool read(uintptr_t address,T& output) {
    SIZE_T size{};return address && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),&output,sizeof(T),&size) && size==sizeof(T);
}
inline Object lookup(uintptr_t table,const char* name) {
    uint32_t mask{};uintptr_t nodes{};
    if(!read(table+0x18,mask)||!read(table+0x20,nodes)||mask>65535)return {};
    for(unsigned i=0;i<=mask;i++) {
        Node node{};if(!read(nodes+i*sizeof(Node),node))return {};
        if((node.key.type&15)!=4)continue;
        char key[96]{};if(!read(node.key.value+20,key))continue;
        if(strnlen_s(key,96)<96 && strcmp(key,name)==0)return node.value;
    }
    return {};
}
}
