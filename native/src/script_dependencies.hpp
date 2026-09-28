#pragma once
#include "stack_logic.hpp"
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>

namespace script_dependencies {
inline uint16_t read16(std::span<const unsigned char> bytes,size_t offset) {
    if(offset>bytes.size() || bytes.size()-offset<2)throw std::runtime_error("Truncated script metadata");
    uint16_t value;memcpy(&value,bytes.data()+offset,2);return value;
}
inline uint32_t read32(std::span<const unsigned char> bytes,size_t offset) {
    if(offset>bytes.size() || bytes.size()-offset<4)throw std::runtime_error("Truncated script dependency table");
    uint32_t value;memcpy(&value,bytes.data()+offset,4);return value;
}
inline std::vector<std::string> includes(std::span<const unsigned char> bytes) {
    constexpr unsigned char magic[]{0x80,'G','S','C',13,10,0,0x1c};
    if(bytes.size()<0x48 || memcmp(bytes.data(),magic,8))throw std::runtime_error("Unsupported compiled script header");
    const auto offset=read32(bytes,12);
    std::vector<std::string> result;
    for(unsigned i=0;i<bytes[0x44];++i) {
        const auto at=read32(bytes,size_t(offset)+i*4);
        if(at>=bytes.size())throw std::runtime_error("Invalid script include offset");
        const auto end=std::find(bytes.begin()+at,bytes.end(),0);
        if(end==bytes.end())throw std::runtime_error("Unterminated script include");
        const std::string name(bytes.begin()+at,end);
        if(!name.starts_with("scripts/") || !stack_logic::safe_resource(name))throw std::runtime_error("Invalid script include name");
        result.push_back(name);
    }
    return result;
}
// Append strings and a new include table. Bytecode, imports, exports, and all
// existing relative addresses remain byte-for-byte at their original offsets.
inline std::vector<unsigned char> merge(std::span<const unsigned char> original,const std::vector<std::string>& additions) {
    auto names=includes(original);
    std::vector<unsigned char> result(original.begin(),original.end());
    std::vector<uint32_t> offsets;
    const auto old_table=read32(original,12);
    for(size_t i=0;i<names.size();++i)offsets.push_back(read32(original,old_table+i*4));
    for(const auto& name:additions) {
        if(std::find(names.begin(),names.end(),name)!=names.end())continue;
        if(!name.starts_with("scripts/") || !stack_logic::safe_resource(name))throw std::runtime_error("Invalid additional script include");
        if(names.size()==255)throw std::runtime_error("Too many startup script dependencies");
        names.push_back(name);offsets.push_back(static_cast<uint32_t>(result.size()));
        result.insert(result.end(),name.begin(),name.end());result.push_back(0);
    }
    if(names.size()==original[0x44])return result;
    while(result.size()%4)result.push_back(0);
    const uint32_t table=static_cast<uint32_t>(result.size());
    for(const auto value:offsets) {
        const auto bytes=reinterpret_cast<const unsigned char*>(&value);
        result.insert(result.end(),bytes,bytes+4);
    }
    memcpy(result.data()+12,&table,4);result[0x44]=static_cast<unsigned char>(names.size());
    return result;
}
// Used only after both complete source fingerprints match the inspected pair.
// Retain Nacht's map initialization and add Ultimis's existing character-init
// call before Nacht registers its callbacks. No creator bytecode is bundled.
inline std::vector<unsigned char> nacht_ultimis(std::span<const unsigned char> nacht,std::span<const unsigned char> ultimis) {
    if(nacht.size()!=1016 || ultimis.size()!=752 || read32(nacht,0x180)!=0x200e200d ||
       read32(ultimis,0x140)!=0x200e200d || read16(ultimis,0x144)!=0x25a4 || read16(ultimis,0x150)!=0x271d)
        throw std::runtime_error("Startup call layout changed");
    auto result=merge(nacht,includes(ultimis));
    size_t export_entry=0;
    for(unsigned i=0;i<read16(nacht,58);++i) {
        const size_t at=read32(nacht,32)+i*20;
        if(read32(nacht,at+8)==0x8c87d8eb && read32(nacht,at+12)==0x8c2d92de)export_entry=at;
    }
    if(!export_entry || read32(nacht,export_entry+4)!=0x180)throw std::runtime_error("Nacht initializer export changed");
    const size_t imports_begin=read32(nacht,36);
    size_t imports_end=imports_begin;
    const auto count=read16(nacht,60);
    if(count==65535)throw std::runtime_error("Startup import count overflow");
    for(unsigned i=0;i<count;++i)imports_end+=12+size_t(read16(nacht,imports_end+8))*4;
    if(imports_end>nacht.size())throw std::runtime_error("Invalid startup imports");
    size_t character_import=0,at=read32(ultimis,36);
    for(unsigned i=0;i<read16(ultimis,60);++i) {
        const auto references=read16(ultimis,at+8);
        if(read32(ultimis,at)==0xc35e6aab && read32(ultimis,at+4)==0x616c194f &&
           references==1 && read32(ultimis,at+12)==0x144)character_import=at;
        at+=12+size_t(references)*4;
    }
    if(!character_import || at>ultimis.size())throw std::runtime_error("Character initializer import changed");
    while(result.size()%16)result.push_back(0);
    const uint32_t stub=static_cast<uint32_t>(result.size());
    result.insert(result.end(),ultimis.begin()+0x140,ultimis.begin()+0x152);
    // Jump resumes at Nacht's original PreScriptCall. Relative operands are
    // signed 16-bit values measured from the end of the operand in T7 GSC.
    const int delta=0x182-static_cast<int>(result.size()+4);
    if(delta<INT16_MIN || delta>INT16_MAX)throw std::runtime_error("Startup bridge jump is out of range");
    const uint16_t jump[]{0x22,static_cast<uint16_t>(delta)};
    const auto bytes=reinterpret_cast<const unsigned char*>(jump);
    result.insert(result.end(),bytes,bytes+4);
    const uint32_t code_size=static_cast<uint32_t>(result.size())-read32(nacht,20);
    while(result.size()%4)result.push_back(0);
    const uint32_t imports=static_cast<uint32_t>(result.size());
    result.insert(result.end(),nacht.begin()+imports_begin,nacht.begin()+imports_end);
    result.insert(result.end(),ultimis.begin()+character_import,ultimis.begin()+character_import+12);
    const uint32_t reference=stub+4;
    const auto refbytes=reinterpret_cast<const unsigned char*>(&reference);
    result.insert(result.end(),refbytes,refbytes+4);
    const uint16_t total=count+1;
    memcpy(result.data()+export_entry+4,&stub,4);
    memcpy(result.data()+36,&imports,4);memcpy(result.data()+60,&total,2);
    memcpy(result.data()+48,&code_size,4);
    return result;
}
}
