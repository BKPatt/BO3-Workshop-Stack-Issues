#include "t7_compat.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main() {
    int continuation{},other{};
    void* slot{};
    check(t7_compat::initialize_null_slot(&slot,&continuation)==t7_compat::Result::repaired,"Null continuation not initialized");
    check(t7_compat::initialize_null_slot(&slot,&continuation)==t7_compat::Result::already_valid,"Repeated repair changed a correct continuation");
    slot=&other;
    check(t7_compat::initialize_null_slot(&slot,&continuation)==t7_compat::Result::occupied && slot==&other,"Existing unknown pointer overwritten");
    slot=nullptr;
    check(t7_compat::initialize_null_slot(&slot,nullptr)==t7_compat::Result::unsupported && !slot,"Missing OS export accepted");
    std::atomic<unsigned> repaired{};
    std::vector<std::thread> workers;
    for(int i=0;i<16;i++)workers.emplace_back([&]{if(t7_compat::initialize_null_slot(&slot,&continuation)==t7_compat::Result::repaired)++repaired;});
    for(auto& worker:workers)worker.join();
    check(repaired==1 && slot==&continuation,"Concurrent notifications did not initialize exactly once");
    wchar_t correct[]=L"T7PATCH.DLL",wrong[]=L"t7patch.dlx";
    UNICODE_STRING name{22,24,correct};
    check(t7_compat::is_t7(&name),"Case-insensitive target name rejected");
    name.Buffer=wrong;check(!t7_compat::is_t7(&name),"Different DLL name accepted");
    name.Length=20;check(!t7_compat::is_t7(&name),"Truncated name accepted");
    std::vector<unsigned char> image(t7_compat::image_size);
    check(t7_compat::repair_image(image.data(),image.size(),&continuation)==t7_compat::Result::unsupported,"Unknown image accepted");
    check(t7_compat::repair_image(image.data(),128,&continuation)==t7_compat::Result::unsupported,"Short image accepted");
    std::cout<<"Null-only initialization, concurrent repair, name validation, and unknown-image refusal passed.\n";
}
