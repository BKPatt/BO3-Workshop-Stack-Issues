#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include "compat_sites_24784313.hpp"

namespace game_build {
// Keep the prior build for users downpatching for old T7. Add profiles; do not
// replace existing identities or infer addresses from a version/range delta.
struct Addresses {
    uintptr_t script_error=0x12ea450;
    uintptr_t allocate_asset=0x13e9e50;
    uintptr_t asset_zone_name=0x1420490;
    uintptr_t link_asset=0x14228e0;
    uintptr_t load_assets=0x14236c0;
    uintptr_t prepare_return=0x1423a0c;
    uintptr_t schedule_return=0x1423a18;
    uintptr_t schedule_zones=0x1423a70;
    uintptr_t asset_priority=0x1423d20;
    uintptr_t try_load=0x1425030;
    uintptr_t prepare_zones=0x1426970;
    uintptr_t package_require=0x1d1cb20;
    uintptr_t lua_to_string=0x1d3f2f0;
    uintptr_t lua_load_buffer=0x1d3f9b0;
    uintptr_t lua_pcall=0x1d41fc0;
    uintptr_t menu_info=0x1fdd300;
    uintptr_t menu_set_mod=0x1fdd570;
    uintptr_t zone_path=0x20c96e0;
    uintptr_t check_mod_changes=0x20c9b80;
    uintptr_t load_mod_by_id=0x20ca870;
    uintptr_t game_error=0x20ec0b0;
    uintptr_t mod_assets_busy=0x20f3220;
    uintptr_t zone_dependency=0x20fc5a0;
    uintptr_t draw_mod_label=0x2230980;
    uintptr_t open_sound_bank=0x2513720;
    uintptr_t script_vm=0x5124840;
    uintptr_t script_position=0x513dd30;
    uintptr_t asset_pools=0x93883f0;
    uintptr_t active_mod=0x1670dd20;
    uintptr_t mod_load_state=0x1670e1e8;
    uintptr_t mods_pool=0x1670e1f0;
};
inline constexpr Addresses legacy_addresses{};
inline constexpr Addresses updated_addresses=[] {
    Addresses result;
    result.package_require=0x1d1c6c0;
    result.lua_to_string=0x1d3ec30;
    result.lua_load_buffer=0x1d3f2f0;
    result.lua_pcall=0x1d41900;
    result.menu_info=0x1fdcc40;
    result.menu_set_mod=0x1fdceb0;
    result.zone_path=0x20c9020;
    result.check_mod_changes=0x20c94c0;
    result.load_mod_by_id=0x20ca1b0;
    result.game_error=0x20eb9f0;
    result.mod_assets_busy=0x20f2b60;
    result.zone_dependency=0x20fbee0;
    result.draw_mod_label=0x22302c0;
    result.open_sound_bank=0x2513060;
    return result;
}();
static_assert(std::size(compat_sites)==std::size(compat_sites_24784313));
using Sites=std::span<const CompatSite,std::size(compat_sites)>;
struct Profile {
    const char* steam_build;
    const char* sha256;
    uint32_t checksum,image_size;
    Addresses addresses;
    Sites sites;
};
inline constexpr Profile legacy{
    "21201493", "9ba98dba41e18ef47de6c63937340f8eae7cb251f8fbc2e78d70047b64aa15b5",
    0x6517980,0x1d74b000,legacy_addresses,compat_sites
};
inline constexpr Profile updated{
    "24784313", "51ca63bbc660e0826943c60da67606f6bcb4b3b519528b5e0548c68c9423a323",
    0x6531394,0x1d75bc00,updated_addresses,compat_sites_24784313
};
inline constexpr std::array profiles{&legacy,&updated};
inline constexpr const Profile* find(std::string_view hash) {
    for(const auto profile:profiles)if(hash==profile->sha256)return profile;
    return nullptr;
}
inline constexpr bool matches_image(const Profile& profile,uint32_t checksum,uint32_t size) {
    return checksum==profile.checksum && size==profile.image_size;
}
// Assigned once, after exact file and mapped-image validation, before installing
// hooks. The legacy default also permits isolated callback regression fixtures.
inline Addresses addresses=legacy_addresses;
inline std::string unsupported_message(const std::string& observed) {
    std::string result="Unsupported BO3 executable. No game patches were applied.\n\nObserved SHA-256: "+observed+"\nSupported Windows Steam executables:";
    for(const auto profile:profiles)result+="\nBuild "+std::string(profile->steam_build)+" - Expected SHA-256: "+profile->sha256;
    result+="\nDownpatching to the exact supported build 21201493 for old T7 is supported. Other executable hashes are not supported.";
    return result;
}
}
