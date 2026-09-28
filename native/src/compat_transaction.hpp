#pragma once
#include <array>
#include <cstddef>
#include <cstring>
#include <span>
#include "compat_sites.hpp"

namespace native_compat {
inline constexpr unsigned char signatures[3][8]={
    {0x8b,0x0c,0x8b,0x33,0x0c,0x82},
    {0x8b,0x0c,0x8b,0xf7,0xd9,0x03,0x0c,0x82},
    {0x8b,0x04,0x82,0x8b,0x14,0x8b}
};
inline constexpr unsigned char replacements[3][8]={
    {0x48,0x31,0xc9,0x90,0x90,0x90},
    {0x48,0x31,0xc9,0x90,0x90,0x90,0x90,0x90},
    {0x48,0x31,0xc0,0x48,0x31,0xd2}
};
inline constexpr size_t length(unsigned kind){return kind==2?8:6;}
enum class SiteState { original, compatible, unknown, unreadable };
enum class PatchError { none, unknown_bytes, unreadable, write_failed, active_transaction };

// No allocation occurs here: the caller can hold the game's threads suspended.
// A recognized patch is shared with its existing owner, never claimed or undone.
template<size_t N> struct Transaction {
    std::array<bool,N> owned{};
    std::array<SiteState,N> observed{};
    size_t applied{},reused{};
    unsigned failed_rva{};
    PatchError error{PatchError::none};
    bool rollback_ok{true};

    template<class Read> static SiteState inspect(const CompatSite& site,Read& read) {
        if(site.kind<1 || site.kind>3)return SiteState::unknown;
        unsigned char bytes[8]{};
        if(!read(site.rva,bytes,length(site.kind)))return SiteState::unreadable;
        if(!memcmp(bytes,signatures[site.kind-1],length(site.kind)))return SiteState::original;
        if(!memcmp(bytes,replacements[site.kind-1],length(site.kind)))return SiteState::compatible;
        return SiteState::unknown;
    }
    template<class Read,class Write> bool rollback(std::span<const CompatSite,N> sites,Read& read,Write& write) {
        bool ok=true;
        for(size_t i=N;i-->0;)if(owned[i]) {
            const auto state=inspect(sites[i],read);
            if(state==SiteState::original)owned[i]=false;
            else if(state==SiteState::compatible && write(sites[i].rva,signatures[sites[i].kind-1],length(sites[i].kind)))owned[i]=false;
            else ok=false; // Preserve any later, unrecognized modification.
        }
        return ok;
    }
    template<class Read,class Write> bool apply(std::span<const CompatSite,N> sites,Read& read,Write& write) {
        for(bool value:owned)if(value){error=PatchError::active_transaction;return false;}
        applied=reused=0;failed_rva=0;error=PatchError::none;rollback_ok=true;
        // Validate the complete set before writing anything, including mixed
        // original/prepatched states left by a compatible native patch.
        for(size_t i=0;i<N;i++) {
            observed[i]=inspect(sites[i],read);
            if(observed[i]==SiteState::unknown || observed[i]==SiteState::unreadable) {
                failed_rva=sites[i].rva;
                error=observed[i]==SiteState::unreadable?PatchError::unreadable:PatchError::unknown_bytes;
                return false;
            }
            if(observed[i]==SiteState::compatible)++reused;
        }
        for(size_t i=0;i<N;i++)if(observed[i]==SiteState::original) {
            // Include the current site if a write copies bytes but cannot
            // restore its page protection. Rollback inspects it again.
            owned[i]=true;
            if(!write(sites[i].rva,replacements[sites[i].kind-1],length(sites[i].kind))) {
                failed_rva=sites[i].rva;error=PatchError::write_failed;
                rollback_ok=rollback(sites,read,write);return false;
            }
            ++applied;
        }
        return true;
    }
};
}
