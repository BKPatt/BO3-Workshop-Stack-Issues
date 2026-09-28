#include "script_dependencies.hpp"
#include <iostream>
#include <fstream>

void check_dependencies(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void verify_pair(const std::vector<unsigned char>& nacht,const std::vector<unsigned char>& ultimis) {
    const auto merged=script_dependencies::nacht_ultimis(nacht,ultimis);
    const auto u32=[&](size_t at){return script_dependencies::read32(merged,at);};
    const auto exp=script_dependencies::read32(nacht,32);
    size_t initializer=0;
    for(unsigned i=0;i<script_dependencies::read16(nacht,58);++i)
        if(script_dependencies::read32(nacht,exp+i*20+8)==0x8c87d8eb)initializer=exp+i*20;
    const auto stub=u32(initializer+4);
    check_dependencies(stub%16==0 && stub>=nacht.size(),"Initializer bridge is misaligned or overwrites creator code");
    check_dependencies(std::equal(ultimis.begin()+0x140,ultimis.begin()+0x152,merged.begin()+stub),"Character call did not retain its original instructions");
    check_dependencies(script_dependencies::read16(merged,stub+18)==0x22 && int(stub+22)+static_cast<int16_t>(script_dependencies::read16(merged,stub+20))==0x182,"Initializer does not resume Nacht's original callbacks");
    const auto code=script_dependencies::read32(nacht,20),length=script_dependencies::read32(nacht,48);
    check_dependencies(std::equal(nacht.begin()+code,nacht.begin()+code+length,merged.begin()+code),"Nacht's original code was changed");
    size_t at=u32(36);bool found=false;
    for(unsigned i=0;i<script_dependencies::read16(merged,60);++i) {
        const auto n=script_dependencies::read16(merged,at+8);
        if(u32(at)==0xc35e6aab && u32(at+4)==0x616c194f)found=n==1 && u32(at+12)==stub+4;
        at+=12+size_t(n)*4;
    }
    check_dependencies(found,"Character call has no matching linker import");
    auto names=script_dependencies::includes(merged);
    for(const auto& name:script_dependencies::includes(nacht))check_dependencies(std::find(names.begin(),names.end(),name)!=names.end(),"Lost Nacht include");
    for(const auto& name:script_dependencies::includes(ultimis))check_dependencies(std::find(names.begin(),names.end(),name)!=names.end(),"Lost Ultimis include");
}
int main(int argc,char** argv) {
    std::vector<unsigned char> script(128,0x5a);
    const unsigned char magic[]{0x80,'G','S','C',13,10,0,0x1c};
    memcpy(script.data(),magic,8);script[0x44]=0;
    const uint32_t table=72;memcpy(script.data()+12,&table,4);
    auto first=script_dependencies::merge(script,{"scripts/zm/first","scripts/shared/system_shared"});
    auto combined=script_dependencies::merge(first,{"scripts/shared/system_shared","scripts/zm/second"});
    check_dependencies(script_dependencies::includes(combined)==std::vector<std::string>{"scripts/zm/first","scripts/shared/system_shared","scripts/zm/second"},"Startup dependencies lost order or contained duplicates");
    for(size_t i=0;i<first.size();++i)if((i<12 || i>=16) && i!=0x44)
        check_dependencies(combined[i]==first[i],"Existing script bytecode or metadata moved/changed");
    check_dependencies(script_dependencies::merge(combined,{"scripts/zm/second"})==combined,"Repeated assembly changed the script");
    std::vector<std::string> many;
    for(int i=0;i<255;++i)many.push_back("scripts/fixture_"+std::to_string(i));
    auto full=script_dependencies::merge(script,many);
    check_dependencies(script_dependencies::includes(full).size()==255,"Maximum representable include count failed");
    bool refused=false;try{script_dependencies::merge(full,{"scripts/overflow"});}catch(...){refused=true;}
    check_dependencies(refused,"Include count wrapped around");
    for(int kind=0;kind<4;++kind) {
        auto bad=combined;
        if(kind==0)bad.resize(60);
        if(kind==1)bad[7]=0;
        if(kind==2)memset(bad.data()+12,0xff,4);
        if(kind==3){const auto at=script_dependencies::read32(bad,12);memset(bad.data()+at,0xff,4);}
        refused=false;try{script_dependencies::includes(bad);}catch(...){refused=true;}
        check_dependencies(refused,"Malformed script metadata was accepted");
    }
    refused=false;try{script_dependencies::merge(script,{"scripts/../escape"});}catch(...){refused=true;}
    check_dependencies(refused,"Unsafe include accepted");
    if(argc==3) {
        const auto read=[](const char* path){std::ifstream f(path,std::ios::binary);return std::vector<unsigned char>(std::istreambuf_iterator<char>(f),{});};
        verify_pair(read(argv[1]),read(argv[2]));
        std::cout<<"Locally captured startup scripts: both code bodies and dependencies retained; character call linked; branch target verified.\n";
    } else {
        auto nacht=script; nacht.resize(1016);auto ultimis=script;ultimis.resize(752);
        const auto w32=[](auto& b,size_t at,uint32_t v){memcpy(b.data()+at,&v,4);};
        const auto w16=[](auto& b,size_t at,uint16_t v){memcpy(b.data()+at,&v,2);};
        for(auto* b:{&nacht,&ultimis}) {w16(*b,58,1);w16(*b,60,1);w32(*b,20,0x140);w32(*b,48,0x160);}
        w32(nacht,32,800);w32(nacht,804,0x180);w32(nacht,808,0x8c87d8eb);w32(nacht,812,0x8c2d92de);
        w32(nacht,36,840);w16(nacht,848,1);w32(nacht,852,0x184);w32(nacht,0x180,0x200e200d);
        w32(ultimis,36,640);w32(ultimis,640,0xc35e6aab);w32(ultimis,644,0x616c194f);w16(ultimis,648,1);w32(ultimis,652,0x144);
        w32(ultimis,0x140,0x200e200d);w16(ultimis,0x144,0x25a4);w16(ultimis,0x150,0x271d);
        verify_pair(nacht,ultimis);
        auto changed=nacht;changed[0x180]^=1;
        refused=false;try{script_dependencies::nacht_ultimis(changed,ultimis);}catch(...){refused=true;}
        check_dependencies(refused,"Changed initialization layout was adapted");
    }
    std::cout<<"Startup include union preserves code addresses, deduplicates dependencies, retains order, and rejects malformed/overflow metadata.\n";
}
