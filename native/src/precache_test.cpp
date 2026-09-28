#include "extension.cpp"
#include <iostream>

void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
precache::Record record(uint32_t type,const char* name) {
    precache::Record value{};value.type=type;strcpy_s(value.name,name);return value;
}
WorkshopData mod(const char* id){WorkshopData result{};strcpy_s(result.publisher_id,id);return result;}
AssetEntry* passthrough(AssetEntry* entry,int){return entry;}
int main() {
    try {
        stack_initialized=true;stack={mod("11"),mod("22")};link_asset=passthrough;
        for(const bool reverse:{false,true}) {
            precache_sources.clear();
            precache::Records weapons{record(7,"custom_rifle_zm"),record(2,"tag_origin")};
            precache::Records characters{record(2,"custom_hands"),record(2,"tag_origin"),record(19,"custom_rifle_zm")};
            precache::Header a{"zm_mod",weapons.data(),2,0},b{"zm_mod",characters.data(),3,0};
            AssetEntry first{85,0,reverse?static_cast<void*>(&b):&a,{}},second{85,0,reverse?static_cast<void*>(&a):&b,{}};
            current_extra=reverse?"22":"11";asset_hook(&first,0);
            current_extra=reverse?"11":"22";asset_hook(&second,0);
            const auto merged=static_cast<precache::Header*>(second.header);
            check(merged->count==4,"A provider or duplicate category was lost");
            check(merged->records[0].type==2 && merged->records[2].type==7 && merged->records[3].type==19,"Native type order was not preserved");
            const auto stable=merged->records;
            weapons.clear();characters.clear();
            check(!strcmp(stable[2].name,"custom_rifle_zm"),"Merged cache refers to released zone records");
        }
        check(retained_precaches.size()==1,"Identical reloads accumulated owned lists");
        stack={mod("33")};precache::Records clean{record(7,"clean_zm")};precache::Header fresh{"zm_mod",clean.data(),1,0};
        current_extra="33";AssetEntry entry{85,0,&fresh,{}};asset_hook(&entry,0);
        check(fresh.count==1 && fresh.records==clean.data(),"Deselected providers leaked into a new stack");
        auto invalid=record(7,"x");memset(invalid.name,'x',128);precache::Header bad{"zm_mod",&invalid,1,0};
        bool refused=false;try{precache::copy(bad);}catch(const std::exception&){refused=true;}
        check(refused,"Unterminated names were accepted");
        bad.records=nullptr;refused=false;try{precache::copy(bad);}catch(const std::exception&){refused=true;}
        check(refused,"Null nonempty lists were accepted");
        // Diagnostic header reuse replaces its previous range, without growing
        // on every DB reload. A swapped header can be registered separately.
        struct Script {const char* name;uint32_t length,padding;const unsigned char* buffer;};
        unsigned char script_bytes[128]{};Script script{"scripts/test.gsc",128,0,script_bytes};
        AssetEntry script_entry{54,0,&script,{}};
        for(int i=0;i<100;++i)remember_script(&script_entry);
        check(script_ranges.size()==1,"Diagnostic ranges accumulated on header reuse");
        script.buffer=nullptr;remember_script(&script_entry);
        check(script_ranges.empty(),"Invalid reused header retained stale diagnostic range");
        std::cout<<"Production asset callback preserves both providers, categories, reload lifetime, and selection isolation\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
