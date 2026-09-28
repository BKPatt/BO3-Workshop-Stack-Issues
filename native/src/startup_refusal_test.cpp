#include <windows.h>
#include <iostream>
#include <stdexcept>
#include "status.hpp"

void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int wmain(int argc,wchar_t** argv) {
    check(argc==2,"Expected the release DLL path");
    // The fixture is intentionally not BO3. The real DLL must reject its hash
    // before dereferencing any game addresses or installing a single hook.
    stack_status::Signals signals(GetCurrentProcessId());
    stack_status::SharedDetails details(GetCurrentProcessId());
    check(static_cast<bool>(signals),"Cannot create test status events");
    const auto module=LoadLibraryW(argv[1]);
    check(module!=nullptr,"Cannot load the release DLL");
    const HANDLE events[]{signals.failed,signals.ready};
    check(WaitForMultipleObjects(2,events,FALSE,10000)==WAIT_OBJECT_0,"Unsupported host did not fail safely");
    const auto message=details.message();
    check(message.find("Unsupported BO3 executable")!=std::string::npos,"Specific startup reason was lost");
    check(message.find("Observed SHA-256: ")!=std::string::npos && message.find("Expected SHA-256: ")!=std::string::npos,"Build identification missing");
    check(message.find("No game patches were applied")!=std::string::npos,"Refusal phase was not reported");
    check(WaitForSingleObject(signals.ready,0)==WAIT_TIMEOUT,"Rejected DLL incorrectly reported ready");
    // Keep the DLL loaded until process exit: its startup worker owns its state.
    std::cout<<"Real DLL safely rejected unsupported host and delivered a specific shared diagnostic.\n";
}
