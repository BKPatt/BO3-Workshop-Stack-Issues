#include "extension.cpp"
#include <iostream>
#include <stdexcept>

void check_editor(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
int native_checks{},reloads{},load_state{};
bool assets_busy{};
std::string reloaded_context;
void test_check(int){++native_checks;}
bool test_busy(){return assets_busy;}
void test_reload(int client,const char* context,bool restart){
    check_editor(client==0 && restart,"Reload used unexpected native arguments");
    ++reloads;reloaded_context=context;
}

int main() {
    check_mod_changes=test_check;mod_assets_busy=test_busy;load_mod_by_id=test_reload;mod_load_state=&load_state;
    stack_initialized=true;
    WorkshopData applied{};strcpy_s(applied.publisher_id,"last");stack={applied};
    ui_reload_pending=true;
    mod_change_check_hook(1);
    check_editor(!reloads && ui_reload_pending,"Secondary client consumed pending reload");
    load_state=2;mod_change_check_hook(0);
    check_editor(!reloads && ui_reload_pending,"Reload interrupted an existing transition");
    load_state=0;assets_busy=true;mod_change_check_hook(0);
    check_editor(!reloads && ui_reload_pending,"Reload interrupted asset loading");
    assets_busy=false;mod_change_check_hook(0);mod_change_check_hook(0);
    check_editor(reloads==1 && reloaded_context=="last" && !ui_reload_pending && native_checks==5,"Same-profile reload was skipped, duplicated, or bypassed native updates");
    check_editor(lua_bindings::boolean_true({1,0,0x27a00000001}),"HKS true rejected stale upper union bytes");
    check_editor(!lua_bindings::boolean_true({1,0,0x27a00000000}),"HKS false accepted stale upper union bytes");
    check_editor(!lua_bindings::boolean_true({3,0,1}),"Non-boolean accepted as frontend permission");
    struct Mod {std::string id;bool gameplay=true;};
    std::vector<Mod> installed{{"first"},{"second"},{"third"},{"loader",false},{"map",false}};
    const auto key=[](const auto& mod){return std::string_view(mod.id);};
    const auto valid=[](const auto& mod){return mod.gameplay;};
    const auto resolve=[&](std::string_view value){return selection_editor::resolve<Mod>(value,installed,key,valid);};
    auto result=resolve("third,first,second");
    check_editor(result && result->size()==3 && (*result)[0].id=="third" && (*result)[1].id=="first" && (*result)[2].id=="second","Explicit order was not preserved");
    result=resolve("second");
    check_editor(result && result->size()==1 && result->front().id=="second","Removing individual providers failed");
    result=resolve("");
    check_editor(result && result->empty(),"Empty selection must unload the stack");
    for(const auto bad:{"first,missing","first,first","first,","first,,second",",first","first,loader","map","../bad","usermaps","first/second","first\nsecond"})
        check_editor(!resolve(bad),"Malformed, duplicate or non-gameplay provider accepted");
    for(int n=0;n<1024;++n)installed.push_back({"fixture_"+std::to_string(n)});
    std::string large;
    for(int n=1023;n>=0;--n){if(!large.empty())large+=',';large+="fixture_"+std::to_string(n);}
    result=resolve(large);
    check_editor(result && result->size()==1024 && result->front().id=="fixture_1023" && result->back().id=="fixture_0","Editor introduced a selection cap or reordered a large request");
    check_editor(selection_editor::lua_string("a\"\\\n\0019")=="\"a\\034\\092\\010\\0019\"","Lua source escaping is unsafe or ambiguous");
    check_editor(selection_editor::lua_string(std::string("\xc3\xa9"))=="\"\\195\\169\"","UTF-8 bytes were not preserved");
    std::cout<<"Ordered replacement, removal, empty selection, invalid requests, 1,024 providers, and safe Lua serialization passed.\n";
}
