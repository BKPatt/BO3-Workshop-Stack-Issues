#include "t7_compat.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void* pointer(HMODULE dll){return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(dll)+t7_compat::slot_rva);}
int wmain(int argc,wchar_t** argv) {
    check(argc==2,"Expected a path to the independently obtained T7 3.04 DLL");
    const auto expected=reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"ZwContinue"));
    // Do not invoke T7's patch-install export. These are real Windows module
    // lifecycle tests in an isolated fixture process, with no BO3 interaction.
    auto dll=LoadLibraryW(argv[1]);check(dll!=nullptr,"Could not load T7 DLL");
    check(pointer(dll)==nullptr,"Expected the original T7 missing continuation defect");
    {
        std::vector<unsigned char> copy(t7_compat::image_size);
        memcpy(copy.data(),dll,copy.size());
        copy[t7_compat::handler_begin]^=1;
        check(t7_compat::repair_image(copy.data(),copy.size(),expected)==t7_compat::Result::unsupported,"Changed handler accepted");
        check(!*reinterpret_cast<void**>(copy.data()+t7_compat::slot_rva),"Unknown handler was modified");
    }
    t7_compat::Adapter adapter;
    check(adapter.start(),"Could not register Windows load notifications");
    check(pointer(dll)==expected && adapter.repaired==1,"Already loaded T7 was not repaired");
    FreeLibrary(dll);
    for(int i=0;i<3;i++) {
        dll=LoadLibraryW(argv[1]);check(dll!=nullptr,"Reload failed");
        check(pointer(dll)==expected,"Late-load notification did not repair before LoadLibrary returned");
        FreeLibrary(dll);
    }
    check(adapter.repaired==4,"Unload/reload lost notifications");
    adapter.stop();
    dll=LoadLibraryW(argv[1]);check(dll!=nullptr,"Final load failed");
    check(pointer(dll)==nullptr,"Unregistered listener still modified T7");
    FreeLibrary(dll);
    std::cout<<"Original T7 null defect reproduced; existing load and three late loads repaired synchronously; unregister passed.\n";
}
