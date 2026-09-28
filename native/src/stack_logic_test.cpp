#include "stack_logic.hpp"
#include <iostream>
#include <stdexcept>
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
    using namespace stack_logic;
    const auto key=[](const auto& v){return v;};
    std::vector<std::string> chosen;
    select(chosen,std::string("2602139181"),key);
    for(int i=0;i<20;i++)select(chosen,std::string("2602139181"),key);
    select(chosen,std::string("2242220555"),key);
    check(chosen==std::vector<std::string>{"2602139181","2242220555"},"Repeated engine polling duplicated or removed a mod");
    select(chosen,std::string("third"),key);select(chosen,std::string("fourth"),key);
    select(chosen,std::string("fifth"),key);select(chosen,std::string("fifth"),key);
    check(chosen==std::vector<std::string>{"2602139181","2242220555","third","fourth","fifth"},"Fifth selection replaced or reordered a prior mod");
    for(int i=0;i<4096;i++)check(select(chosen,"bulk_"+std::to_string(i),key),"A new selection was rejected");
    check(chosen.size()==4101 && chosen[3]=="fourth" && chosen[4]=="fifth","Large stack lost earlier selections");
    const auto original=chosen;
    for(int i=4095;i>=0;i--)check(!select(chosen,"bulk_"+std::to_string(i),key),"Polling added a duplicate");
    check(chosen==original,"Polling reordered a large stack");
    for(const int i:{0,4,31,127,1023,4095}) {
        const auto id="bulk_"+std::to_string(i);
        const auto routed=parse_alias("bp_mm"+id+"_zm_mod",chosen);
        check(routed && routed->provider==id && routed->resource=="bp_zm_mod","A large-stack provider was not routed");
    }
    chosen.clear();select(chosen,std::string("fresh"),key);
    check(chosen.size()==1 && chosen[0]=="fresh","Unload retained a stale selection");
    check(!valid_id("") && !valid_id("usermaps") && !valid_id("../bad") && !valid_id(std::string(32,'1')),"Invalid publisher ID accepted");
    check(valid_id("local_mod-1"),"Local identifier rejected");
    const std::vector<std::string> ids{"2602139181","2242220555","local_mod-1"};
    auto alias=parse_alias("en_mm2242220555_zm_mod",ids);
    check(alias && alias->provider=="2242220555" && alias->resource=="en_zm_mod","Localized alias routed to the wrong provider");
    for(const auto& prefix:{"bp","ea","en","es","fr","ge","it","ja","po","ru","sc","tc"}) {
        const std::string localized=std::string(prefix)+"_";
        for(const auto& zone:{"core_mod","zm_mod","mp_mod"}) {
            const auto translated=parse_alias(localized+"mm2242220555_"+zone,ids);
            check(translated && translated->provider=="2242220555" && translated->resource==localized+zone,"A supported language alias lost its prefix/provider");
        }
    }
    alias=parse_alias("mmlocal_mod-1_core_mod",ids);
    check(alias && alias->resource=="core_mod","Underscore in publisher ID broke alias parsing");
    for(const auto& bad:{"zm_owchallenge","zm_common","some_mm2242220555_zm_mod","mm999_zm_mod","mm2242220555_../zm_mod","snd/mm2242220555_zm_mod","mm2242220555_mp_map"})
        check(!parse_alias(bad,ids),"Unrelated map/resource was mistaken for a stack alias");
    for(const auto& bad:{"../zm_mod","a/../../x","/zm_mod","C:/x","a//b","a/./b","\\\\server\\x"})
        check(!safe_resource(bad),"Unsafe resource path accepted");
    check(safe_resource("snd/en/zm_mod.en.sabs") && safe_resource("en_zm_mod"),"Valid resource rejected");
    std::cout<<"4,101 ordered selections, duplicate polling, large-stack routing, clear, publisher validation, and localization passed.\n";
}
