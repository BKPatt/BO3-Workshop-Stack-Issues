#include "game_build.hpp"
#include "compat_transaction.hpp"
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}

int main() {
    using namespace game_build;
    check(find("9ba98dba41e18ef47de6c63937340f8eae7cb251f8fbc2e78d70047b64aa15b5")==&legacy,"Prior Steam build was dropped");
    check(find("51ca63bbc660e0826943c60da67606f6bcb4b3b519528b5e0548c68c9423a323")==&updated,"Updated Steam build was not recognized");
    check(!find("") && !find("21201493") && !find("24784313"),"Build number or failed hash bypassed identity check");
    for(const auto profile:profiles) {
        std::string changed=profile->sha256;changed[0]=changed[0]=='0'?'1':'0';
        check(!find(changed),"Unknown executable hash accepted");
        check(matches_image(*profile,profile->checksum,profile->image_size),"Valid mapped image refused");
        check(!matches_image(*profile,profile->checksum^1,profile->image_size),"Wrong checksum accepted");
        check(!matches_image(*profile,profile->checksum,profile->image_size+0x1000),"Wrong mapped image size accepted");
        std::set<unsigned> rvas;
        std::array<size_t,3> kinds{};
        for(const auto& site:profile->sites) {
            check(site.kind>=1 && site.kind<=3 && site.rva+8<profile->image_size,"Invalid patch site");
            check(rvas.insert(site.rva).second,"Duplicate patch ownership site");
            ++kinds[site.kind-1];
        }
        check(kinds==std::array<size_t,3>{259,259,847},"Incomplete compatibility table");
        // Exercise the production transaction on each full profile: pristine,
        // T7-first, and mixed ownership. Rollback must restore the exact input.
        for(unsigned mode=0;mode<3;++mode) {
            std::map<unsigned,std::array<unsigned char,8>> memory;
            size_t expected_reused{};
            for(size_t i=0;i<profile->sites.size();++i) {
                const auto& site=profile->sites[i];
                const bool prepatched=mode==1 || (mode==2 && i%2==0);
                if(prepatched)++expected_reused;
                const auto source=prepatched?native_compat::replacements[site.kind-1]:native_compat::signatures[site.kind-1];
                std::memcpy(memory[site.rva].data(),source,native_compat::length(site.kind));
            }
            const auto before=memory;
            size_t writes{};
            auto read=[&](unsigned rva,void* out,size_t size){
                const auto found=memory.find(rva);if(found==memory.end())return false;
                std::memcpy(out,found->second.data(),size);return true;
            };
            auto write=[&](unsigned rva,const unsigned char* bytes,size_t size){
                const auto found=memory.find(rva);if(found==memory.end())return false;
                std::memcpy(found->second.data(),bytes,size);++writes;return true;
            };
            native_compat::Transaction<std::size(compat_sites)> tx;
            const auto other=profile==&legacy?&updated:&legacy;
            check(!tx.apply(other->sites,read,write) && writes==0,"Wrong-build patches wrote to memory");
            check(tx.apply(profile->sites,read,write),"Valid native profile refused");
            check(tx.reused==expected_reused && tx.applied+tx.reused==1365,"Build-specific patch ownership lost");
            check(writes==tx.applied,"Preexisting T7 bytes were rewritten");
            check(tx.rollback(profile->sites,read,write) && memory==before,"Rollback damaged another patch owner's bytes");
        }
    }
    check(!matches_image(legacy,updated.checksum,updated.image_size),"Profiles accepted mismatched mapped image");
    check(legacy.addresses.package_require==0x1d1cb20 && updated.addresses.package_require==0x1d1c6c0,"Lua require relocation missing");
    check(legacy.addresses.lua_to_string==0x1d3f2f0 && updated.addresses.lua_to_string==0x1d3ec30,"Lua string relocation missing");
    check(updated.addresses.lua_load_buffer==0x1d3f2f0 && updated.addresses.lua_pcall==0x1d41900,"Lua execution relocation missing");
    check(legacy.addresses.load_mod_by_id==0x20ca870 && updated.addresses.load_mod_by_id==0x20ca1b0,"Native reload relocation missing");
    check(updated.addresses.load_assets==legacy.addresses.load_assets && updated.addresses.mods_pool==legacy.addresses.mods_pool,"Unchanged engine addresses shifted");
    const auto diagnostic=unsupported_message("test-hash");
    check(diagnostic.find(legacy.sha256)!=std::string::npos && diagnostic.find(updated.sha256)!=std::string::npos,"Supported identities missing from refusal");
    std::cout<<"Both exact Steam identities, mapped-image mismatch refusal, and full-profile T7 ownership/rollback passed.\n";
}
