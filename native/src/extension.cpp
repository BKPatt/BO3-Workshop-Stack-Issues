#include <windows.h>
#include <MinHook.h>
#include <array>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <share.h>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>
#include <map>
#include <memory>
#include <atomic>
#include <cctype>
#include <intrin.h>
#include "lua_runtime.hpp"
#include "native_compat.hpp"
#include "stack_logic.hpp"
#include "selection_editor.hpp"
#include "frontend_source.hpp"
#include "stream_routing.hpp"
#include "script_dependencies.hpp"
#include "precache.hpp"
#include "zone_plan.hpp"
#include "asset_capacity.hpp"
#include "asset_priority.hpp"
#include "status.hpp"
#include "build_identity.hpp"
#include "t7_compat.hpp"

namespace {
HMODULE module;
uintptr_t base;
FILE* logfile;
std::mutex log_lock;
t7_compat::Adapter t7_adapter;
void log(const char* format, ...) {
    std::lock_guard lock(log_lock);
    if (!logfile) return;
    fprintf(logfile, "[%llu:%lu] ", GetTickCount64(), GetCurrentThreadId());
    va_list args; va_start(args, format); vfprintf(logfile, format, args); va_end(args);
    fputc('\n', logfile); fflush(logfile);
}
void report_t7() {
    if(const auto count=t7_adapter.repaired.exchange(0))log("T7 COMPAT initialized missing exception continuation pointer: Scroptss 3.04 (%u module loads)",count);
    if(const auto count=t7_adapter.unsupported.exchange(0))log("T7 COMPAT unknown DLL layout; left untouched (%u observations)",count);
    if(const auto count=t7_adapter.occupied.exchange(0))log("T7 COMPAT existing different continuation pointer; left untouched (%u observations)",count);
}
struct WorkshopData {
    char title[100], internal_name[32], publisher_id[32], description[256];
    char content_path[260], content_directory[260], zone_directory[260];
    uint32_t version, padding;
    uint64_t publisher_number;
    uint32_t publisher_hash, type;
};
static_assert(sizeof(WorkshopData)==0x4c8);
std::vector<WorkshopData> snapshot_stack();
std::vector<std::string> provider_ids(const std::vector<WorkshopData>& selected);
using ZoneInfo=stack_logic::ZoneInfo;
using LoadAssets = void(*)(ZoneInfo*, unsigned, int, int);
using TryLoad = int(*)(const char*, int, const void*);
using ZonePath = int(*)(const char*,const char*,int,char*,int,const char*);
using MenuSetMod = int(*)(void*);
struct AssetEntry {int type,padding;void* header; unsigned char rest[16];};
using LinkAsset = AssetEntry*(*)(AssetEntry*,int);
LoadAssets load_assets;
TryLoad try_load;
ZonePath zone_path;
MenuSetMod menu_set_mod;
MenuSetMod menu_info;
using CheckModChanges=void(*)(int);
using LoadModById=void(*)(int,const char*,bool);
CheckModChanges check_mod_changes;
LoadModById load_mod_by_id;
bool(*mod_assets_busy)();
const int* mod_load_state;
std::atomic<bool> ui_reload_pending{};
void mod_change_check_hook(int client) {
    check_mod_changes(client);
    // Run only at BO3's normal mod-update point, after its current transition
    // has finished. Never tear down frontend assets inside a Lua callback.
    if(client!=0 || !ui_reload_pending.load() || *mod_load_state!=0 || mod_assets_busy())return;
    if(ui_reload_pending.exchange(false)) {
        const auto selected=snapshot_stack();
        const auto context=selected.empty()?std::string():std::string(selected.back().publisher_id);
        log("UI RELOAD unchanged profile=%s",context.c_str());
        load_mod_by_id(client,context.c_str(),true);
    }
}
using DrawModLabel=void(*)(int,int);
DrawModLabel draw_mod_label;
void mod_label_hook(int,int) {
    // The stock renderer exposes only one mod and also draws over match HUDs.
    // Our frontend-only Lua prompt replaces it while Workshop Stack is active.
}
LinkAsset link_asset;
using AssetPriority=int(*)(const AssetEntry*,const AssetEntry*);
using AssetZoneName=const char*(*)(const AssetEntry*);
AssetPriority native_asset_priority;
AssetZoneName asset_zone_name;
std::atomic<std::shared_ptr<const asset_priority::Table>> priority_table{std::make_shared<const asset_priority::Table>()};
std::atomic<unsigned> priority_reports{};
MenuSetMod package_require;
using ZoneDependency=bool(*)(const char*,const char*,const char**);
ZoneDependency zone_dependency;
using OpenSoundBank=bool(*)(bool,void*,void*);
OpenSoundBank open_sound_bank;
using ScriptError=void(*)(int,const char*,bool);
ScriptError script_error;
void log_script_frames(int instance);
void script_error_hook(int instance,const char* message,bool terminal) {
    log("SCRIPT ERROR instance=%d terminal=%d %s",instance,terminal,message?message:"<null>");
    log_script_frames(instance);
    script_error(instance,message,terminal);
}
using AllocateAsset=void*(*)(int);
AllocateAsset allocate_asset;
asset_capacity::Storage asset_storage;
using PrepareZones=int(*)(ZoneInfo*,int,char*,ZoneInfo*,int);
using ScheduleZones=void(*)(ZoneInfo*,int);
PrepareZones prepare_zones;
ScheduleZones schedule_zones;
struct PreparedZones {
    std::vector<std::array<char,64>> names;
    std::vector<ZoneInfo> entries;
    ZoneInfo* stock_output{};
    int count{};
};
thread_local PreparedZones prepared_zones;
// Verified at the supported build's DB load error call: file, line, error
// category, format, varargs. ERR_DROP returns the user to the game error flow.
void load_error(const char* message) {
    log("LOAD REFUSED %s",message);
    using Error=void(*)(const char*,int,int,const char*,...);
    reinterpret_cast<Error>(base+game_build::addresses.game_error)("bo3_stack",0,2,"%s",message);
}
int prepare_hook(ZoneInfo* input,int count,char* names,ZoneInfo* output,int capacity) {
    if(reinterpret_cast<uintptr_t>(_ReturnAddress())!=base+game_build::addresses.prepare_return)
        return prepare_zones(input,count,names,output,capacity);
    auto& owned=prepared_zones;
    owned.stock_output=nullptr;
    // The inspected converter emits at most patch, localized patch, localized,
    // and base records per input. Stock DB_LoadXAssets has room for only 32.
    if(count<0 || count>INT_MAX/4) {load_error("The mod stack's package count exceeds BO3's loading interface.");return 0;}
    const auto needed=std::max(32,count*4);
    try {owned.names.resize(needed);owned.entries.resize(needed);}
    catch(...) {load_error("There is not enough memory to prepare this mod stack. Load fewer mods.");return 0;}
    const auto result=prepare_zones(input,count,reinterpret_cast<char*>(owned.names.data()),owned.entries.data(),needed);
    if(result<0 || result>needed || result>64) {
        load_error("This mod stack needs more package slots in one load than BO3's 64-slot zone table supports. Load fewer mods or mods with fewer packages.");
        return 0;
    }
    owned.stock_output=output;owned.count=result;
    log("LOAD BUFFER inputs=%d capacity=%d prepared=%d",count,needed,result);
    return result;
}
void schedule_hook(ZoneInfo* zones,int count) {
    auto& owned=prepared_zones;
    if(reinterpret_cast<uintptr_t>(_ReturnAddress())==base+game_build::addresses.schedule_return && zones==owned.stock_output) {
        owned.stock_output=nullptr;
        if(count!=owned.count) {load_error("The game changed a prepared package batch. Restart BO3 before retrying.");return;}
        zones=owned.entries.data();
    }
    schedule_zones(zones,count);
}
void* allocate_hook(int type) {
    auto result=allocate_asset(type);
    if(result || (type!=47 && type!=54))return result;
    auto& pool=reinterpret_cast<asset_capacity::Pool*>(base+game_build::addresses.asset_pools)[type];
    if(!asset_capacity::supported(type,pool.item_size))return nullptr;
    MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);
    if(!GlobalMemoryStatusEx(&memory))return nullptr;
    const auto bytes=asset_capacity::growth_bytes(pool,memory.ullAvailPhys,memory.ullAvailPageFile,memory.ullTotalPhys);
    // DB already serializes allocation and free operations. Grow at the exact
    // exhaustion point on that same thread, without moving any existing header.
    const auto previous=pool.capacity;
    if(!asset_storage.grow(pool,bytes)) {
        log("MEMORY pool=%d growth refused capacity=%d used=%d available_physical=%llu available_commit=%llu",type,pool.capacity,pool.allocated,memory.ullAvailPhys,memory.ullAvailPageFile);
        return nullptr;
    }
    log("MEMORY pool=%d grew=%d->%d bytes=%zu retained=%zu",type,previous,pool.capacity,bytes,asset_storage.committed);
    return allocate_asset(type);
}
std::mutex stack_lock;
std::vector<WorkshopData> stack;
bool stack_initialized=false;
thread_local std::string current_extra;
thread_local std::string current_zone;
std::mutex stream_lock;
// Keep folder identities through unloading: selection is cleared before the
// engine releases the old zones and enumerates their streaming dependencies.
std::map<std::string,std::string> stream_folders;
std::map<const void*,std::string> sound_owners;
struct DependencyName {std::string provider,name;const char* native;};
std::deque<DependencyName> dependency_names;
std::map<const char*,const char*> dependency_cursors;
void remember_stream_provider(const WorkshopData& mod) {
    std::lock_guard lock(stream_lock);
    stream_folders[mod.publisher_id]=mod.zone_directory;
}
std::vector<std::string> stream_provider_ids() {
    std::lock_guard lock(stream_lock);
    std::vector<std::string> ids;
    for(const auto& [id,folder]:stream_folders)ids.push_back(id);
    return ids;
}
std::string stream_folder(const std::string& id) {
    std::lock_guard lock(stream_lock);
    const auto found=stream_folders.find(id);
    return found==stream_folders.end()?"":found->second;
}
bool dependency_hook(const char* kind,const char* zone,const char** cursor) {
    if(!kind || strcmp(kind,"xpak_read") || !zone || !cursor)return zone_dependency(kind,zone,cursor);
    const auto alias=stack_logic::parse_alias(zone,stream_provider_ids());
    if(!alias)return zone_dependency(kind,zone,cursor);
    // BO3 uses the original value pointer as its iterator cursor. Translate it
    // back before asking for the next item; never pass our copied name to it.
    {
        std::lock_guard lock(stream_lock);
        if(const auto old=dependency_cursors.find(*cursor);old!=dependency_cursors.end())*cursor=old->second;
    }
    if(!zone_dependency(kind,alias->resource.c_str(),cursor))return false;
    const char* original=*cursor;
    if(!original || !stack_logic::safe_resource(original))return true;
    const auto folder=stream_folder(alias->provider);
    const auto exists=[&](const std::string& name){std::error_code ec;return std::filesystem::is_regular_file(std::filesystem::path(folder)/(name+".xpak"),ec);};
    // Localized files are probed by the engine from this same dependency name.
    bool owned=exists(original);
    for(const char* language:{"bp","ea","en","es","fr","ge","it","ja","po","ru","sc","tc"})
        if(!owned)owned=exists(std::string(language)+"_"+original);
    if(!owned)return true;
    const auto name="mm"+alias->provider+"_"+original;
    if(name.size()+3>=64){load_error("A mod streaming package name exceeds BO3's filename buffer. Load fewer or differently packaged mods.");return false;}
    {
        std::lock_guard lock(stream_lock);
        auto found=std::find_if(dependency_names.begin(),dependency_names.end(),[&](const auto& item){return item.provider==alias->provider && item.native==original && item.name==name;});
        if(found==dependency_names.end()) {
            dependency_names.push_back({alias->provider,name,original});
            found=std::prev(dependency_names.end());
            dependency_cursors[found->name.c_str()]=original;
        }
        *cursor=found->name.c_str();
    }
    log("STREAM dependency zone=%s native=%s alias=%s",zone,original,*cursor);
    return true;
}
bool sound_bank_hook(bool streamed,void* asset_bank,void* load) {
    const auto bank=load?*static_cast<void**>(load):nullptr;
    std::string owner;
    {
        std::lock_guard lock(stream_lock);
        if(const auto found=sound_owners.find(bank);found!=sound_owners.end())owner=found->second;
    }
    struct Restore {std::string previous;~Restore(){current_extra=std::move(previous);}} restore{std::move(current_extra)};
    current_extra=owner;
    if(!owner.empty())log("SOUND bank=%p provider=%s streamed=%d",bank,owner.c_str(),streamed);
    return open_sound_bank(streamed,asset_bank,load);
}
std::mutex hud_lock;
struct HudModule { std::string provider,bytes; };
std::map<std::string,std::shared_ptr<HudModule>> hud_modules;
std::vector<std::shared_ptr<HudModule>> retained_hud_modules;
std::atomic<unsigned> hud_generation{1};
std::mutex collision_lock;
struct AssetVersion {std::string provider;uint64_t hash;};
std::map<std::pair<int,std::string>,AssetVersion> asset_versions;
std::map<std::pair<int,std::string>,std::string> named_asset_providers;
uint64_t fingerprint(const unsigned char* data,size_t length) {
    uint64_t hash=14695981039346656037ull;
    for(size_t i=0;i<length;i++){hash^=data[i];hash*=1099511628211ull;}
    return hash;
}
std::mutex startup_script_lock;
struct StartupScript {std::vector<unsigned char> bytes;uint64_t hash;};
std::map<std::string,StartupScript> startup_scripts;
std::vector<std::shared_ptr<std::vector<unsigned char>>> retained_startup_scripts;
struct ScriptRange {uintptr_t address;size_t length;std::string name;};
std::mutex script_range_lock;
std::map<const void*,ScriptRange> script_ranges;
void remember_script(const AssetEntry* entry) {
    if(!entry || entry->type!=54 || !entry->header)return;
    struct Script {const char* name;uint32_t length,padding;const unsigned char* buffer;};
    const auto script=static_cast<const Script*>(entry->header);
    std::lock_guard lock(script_range_lock);
    script_ranges.erase(script);
    if(script->name && script->buffer && script->length>=72 && script->length<16*1024*1024)
        script_ranges[script]={reinterpret_cast<uintptr_t>(script->buffer),script->length,script->name};
}
std::mutex precache_lock;
std::map<std::pair<std::string,std::string>,precache::Records> precache_sources;
std::vector<std::shared_ptr<precache::Records>> retained_precaches;
void combine_precache(AssetEntry* entry) {
    if(entry->type!=85 || !entry->header || current_extra.empty())return;
    auto header=static_cast<precache::Header*>(entry->header);
    if(!header->name || !stack_logic::mod_zone(header->name))return;
    const auto selected=snapshot_stack();
    std::lock_guard lock(precache_lock);
    {
        auto& source=precache_sources[{current_extra,header->name}];
        source=precache::copy(*header);
        std::vector<const precache::Records*> sources;
        for(const auto& mod:selected)
            if(const auto found=precache_sources.find({mod.publisher_id,header->name});found!=precache_sources.end())sources.push_back(&found->second);
        if(sources.size()<2)return;
        auto merged=precache::combine(sources);
        // Lists contain no relocations and the engine reads them without edits.
        // Reuse identical unions across reloads; keep them alive for DB stashes.
        auto retained=std::find_if(retained_precaches.begin(),retained_precaches.end(),[&](const auto& old){
            return old->size()==merged.size() && (merged.empty() || !memcmp(old->data(),merged.data(),merged.size()*sizeof(precache::Record)));
        });
        if(retained==retained_precaches.end()) {
            retained_precaches.push_back(std::make_shared<precache::Records>(std::move(merged)));
            retained=std::prev(retained_precaches.end());
        }
        header->records=(*retained)->data();header->count=static_cast<uint32_t>((*retained)->size());
        log("PRECACHE combined name=%s providers=%zu entries=%u weapons=%zu",header->name,sources.size(),header->count,
            std::count_if((*retained)->begin(),(*retained)->end(),[](const auto& row){return row.type==7;}));
    }
}
bool composed_asset(const AssetEntry* entry) {
    if(entry->type==85) {
        const auto header=static_cast<const precache::Header*>(entry->header);
        if(!header->name || !stack_logic::mod_zone(header->name) || !header->records || !header->count)return false;
        std::lock_guard lock(precache_lock);
        return std::any_of(retained_precaches.begin(),retained_precaches.end(),[&](const auto& records){return header->records==records->data();});
    }
    if(entry->type==54) {
        struct Script {const char* name;uint32_t length,padding;const unsigned char* buffer;};
        const auto header=static_cast<const Script*>(entry->header);
        std::lock_guard lock(startup_script_lock);
        return std::any_of(retained_startup_scripts.begin(),retained_startup_scripts.end(),[&](const auto& bytes){return header->buffer==bytes->data();});
    }
    return false;
}
const asset_priority::Rank* entry_rank(const AssetEntry* entry,const asset_priority::Table& table) {
    if(!entry || !asset_zone_name || entry->rest[0]==0 || entry->rest[0]>=64)return nullptr;
    const auto name=asset_zone_name(entry);
    if(!name)return nullptr;
    const auto length=strnlen_s(name,64);
    return length==64?nullptr:asset_priority::find(table,{name,length});
}
int priority_hook(const AssetEntry* candidate,const AssetEntry* existing) {
    // DB's original comparison returns true when the candidate takes priority.
    // Let DB retain its own header swaps, sorted stashes and unload promotion.
    const auto original=native_asset_priority(candidate,existing);
    if(!candidate || !existing || candidate->type!=existing->type || !candidate->header || !existing->header ||
       candidate->rest[2] || existing->rest[2] || candidate->rest[3] || existing->rest[3])return original;
    const auto table=priority_table.load();
    const auto incoming=entry_rank(candidate,*table),active=entry_rank(existing,*table);
    if(!incoming || !active || incoming->provider==active->provider)return original;
    // Deliberately composed precaches/startup scripts contain both providers.
    // Keep the union active regardless of which mod supplied its final header.
    const bool incoming_composed=composed_asset(candidate),active_composed=composed_asset(existing);
    const auto preferred=incoming_composed!=active_composed?int(incoming_composed):
        (incoming_composed?original:int(incoming->position<active->position));
    if(preferred!=original && priority_reports.fetch_add(1)<64)
        log("PRIORITY type=%d winner=%s lower=%s composed=%d",candidate->type,
            preferred?incoming->provider.c_str():active->provider.c_str(),
            preferred?active->provider.c_str():incoming->provider.c_str(),incoming_composed||active_composed);
    return preferred;
}
void remember_sound_owner(const AssetEntry* entry) {
    if(!entry || entry->type!=10 || !entry->header)return;
    const auto table=priority_table.load();
    const auto owner=entry_rank(entry,*table);
    std::lock_guard lock(stream_lock);
    if(owner)sound_owners[entry->header]=owner->provider;
    else sound_owners.erase(entry->header);
}
void log_script_frames(int instance) {
    if(instance<0 || instance>1)return;
    // Verified in this build's Scr_Error and VM_Execute disassembly. Read only;
    // preserve BO3's error handling and never suppress a failed script call.
    const auto vm=base+game_build::addresses.script_vm+size_t(instance)*0x8a40;
    const auto count=*reinterpret_cast<const int*>(vm+0x10);
    std::lock_guard lock(script_range_lock);
    const auto position=[&](const char* kind,int frame,uintptr_t pos) {
        for(const auto& [header,range]:script_ranges)
            if(pos>=range.address && pos-range.address<range.length) {
                log("SCRIPT FRAME %s %d %s +0x%zx",kind,frame,range.name.c_str(),pos-range.address);return;
            }
        log("SCRIPT FRAME %s %d unresolved=%p",kind,frame,reinterpret_cast<void*>(pos));
    };
    position("current",count,*reinterpret_cast<const uintptr_t*>(base+game_build::addresses.script_position+size_t(instance)*32));
    for(int i=std::clamp(count,0,63);i>=0;--i)
        position("caller",i,*reinterpret_cast<const uintptr_t*>(vm+0x40+size_t(i)*40));
}
std::string canonical(std::string name) {
    for(auto& c:name){if(c=='.'||c=='\\')c='/';else c=static_cast<char>(tolower(static_cast<unsigned char>(c)));}
    if(name.ends_with("/lua"))name.resize(name.size()-4);
    return name;
}
struct Pool { uint32_t count,padding; WorkshopData entries[128]; };
Pool* mods_pool() {return reinterpret_cast<Pool*>(base+game_build::addresses.mods_pool);}
bool is_loader(const WorkshopData& mod) {
    return strncmp(mod.internal_name,"bo3_workshop_stack",sizeof(mod.internal_name))==0;
}
bool valid(const WorkshopData& mod) {
    const auto id=std::string(mod.publisher_id,strnlen_s(mod.publisher_id,32));
    return stack_logic::valid_id(id) && mod.type==1 && !is_loader(mod) && mod.zone_directory[0] && strnlen_s(mod.zone_directory,260)<260;
}
void seed_locked() {
    if(stack_initialized)return;
    const auto active=reinterpret_cast<WorkshopData*>(base+game_build::addresses.active_mod);
    if(valid(*active)) {stack.push_back(*active);stack_initialized=true;log("SEED active mod %.31s",active->publisher_id);}
}
std::vector<WorkshopData> snapshot_stack(){std::lock_guard lock(stack_lock);seed_locked();return stack;}
std::string ui_mods(const std::vector<WorkshopData>& mods) {
    std::string result="{";
    for(const auto& mod:mods) {
        result+="{id="+selection_editor::lua_string(mod.publisher_id)+",title="+
            selection_editor::lua_string(std::string_view(mod.title,strnlen_s(mod.title,sizeof(mod.title))))+"},";
    }
    return result+"}";
}
int ui_request(void* vm,std::string_view request) {
    std::string error;
    const auto run=[&](std::string_view source){return lua_runtime::execute(base,vm,source,"bo3_stack/ui_bridge",error);};
    // Recheck the real VM's map on every request, including read-only requests.
    const auto checked=run("rawset(_G,'BO3StackFrontendAllowed',Engine.GetCurrentMap()=='core_frontend')");
    const auto allowed=lua_runtime::global(vm,"BO3StackFrontendAllowed");
    // HKS booleans occupy the low 32 bits of the value union. Its upper
    // bytes can retain a previous pointer and must not participate in truth.
    if(!checked || !lua_bindings::boolean_true(allowed)) {
        *lua_runtime::top(vm)++=lua_runtime::Object{1,0,0};return 1;
    }
    bool ok=false;
    if(request=="state") {
        std::vector<WorkshopData> installed;
        const auto pool=mods_pool();
        for(unsigned i=0;i<std::min(pool->count,128u);++i)if(valid(pool->entries[i]))installed.push_back(pool->entries[i]);
        ok=run("rawset(_G,'BO3StackNativeState',{active="+ui_mods(snapshot_stack())+",available="+ui_mods(installed)+"})");
    } else if(request.starts_with("diagnostic/")) {
        log("UI CALLBACK ERROR %.*s",static_cast<int>(std::min<size_t>(request.size()-11,2048)),request.data()+11);
        ok=true;
    } else if(request.starts_with("apply/")) {
        const auto pool=mods_pool();
        const auto resolved=selection_editor::resolve<WorkshopData>(request.substr(6),
            {pool->entries,std::min(pool->count,128u)},[](const auto& m){return std::string_view(m.publisher_id);},valid);
        if(resolved) {
            const auto previous=snapshot_stack();
            if(provider_ids(previous)==provider_ids(*resolved))ok=true;
            else {
                {std::lock_guard lock(stack_lock);stack=*resolved;stack_initialized=true;++hud_generation;}
                const auto context=resolved->empty()?std::string():std::string(resolved->back().publisher_id);
                // BO3 compares fs_game with its active publisher each frame.
                // An unchanged last provider needs an explicit deferred reload.
                const auto active=reinterpret_cast<const WorkshopData*>(base+game_build::addresses.active_mod);
                const auto same_context=context==active->publisher_id;
                // Keep the lobby's UGC state synchronized too. Changing only
                // fs_game lets the lobby restore its previous mod after clear.
                ok=same_context || run("Engine.LobbyVM_CallFunc('LoadMod',{ugcName="+
                    selection_editor::lua_string(context)+",ugcType=LuaEnums.MODS_BASE_PATH,ugcVersion=1})");
                if(!ok){std::lock_guard lock(stack_lock);stack=previous;++hud_generation;}
                else {
                    ui_reload_pending.store(same_context);
                    log("UI APPLY count=%zu context=%s",resolved->size(),context.c_str());
                    for(const auto& m:*resolved)log("ENABLED %.31s %.99s",m.publisher_id,m.title);
                }
            }
        }
    }
    if(!error.empty())log("UI ERROR %s",error.c_str());
    *lua_runtime::top(vm)++=lua_runtime::Object{1,0,ok?1u:0u};return 1;
}
std::vector<std::string> provider_ids(const std::vector<WorkshopData>& selected) {
    std::vector<std::string> ids;for(const auto& m:selected)ids.emplace_back(m.publisher_id);return ids;
}
std::string provider_for(const std::string& resource,const std::vector<WorkshopData>& selected) {
    if(!stack_logic::safe_resource(resource))return {};
    for(const auto& m:selected) {
        std::error_code ec;
        if(std::filesystem::is_regular_file(std::filesystem::path(m.zone_directory)/resource,ec))return m.publisher_id;
    }
    return {};
}
int info_hook(void* vm) {
    report_t7();
    const auto selected=snapshot_stack();
    auto pool=mods_pool();
    const auto n=std::min(pool->count,128u);
    std::vector<WorkshopData> backup(pool->entries,pool->entries+n);
    for(unsigned i=0;i<n;i++) {
        if(is_loader(pool->entries[i])) {
            snprintf(pool->entries[i].title,100,"[READY] BO3 Workshop Stack");
            continue;
        }
        const auto it=std::find_if(selected.begin(),selected.end(),[&](const auto& m){return strcmp(m.publisher_id,pool->entries[i].publisher_id)==0;});
        if(it!=selected.end()) snprintf(pool->entries[i].title,100,"[ON %zu] %.86s",size_t(it-selected.begin()+1),backup[i].title);
    }
    const auto result=menu_info(vm);
    for(unsigned i=0;i<n;i++) memcpy(pool->entries[i].title,backup[i].title,100);
    return result;
}

int menu_mod_hook(void* vm) {
    // The menu's first argument is a HavokScript string object. Use its native
    // conversion routine on the calling UI thread; retain BO3's normal transition.
    const auto lua_base=*reinterpret_cast<void**>(static_cast<char*>(vm)+0x50);
    using ToString=const char*(*)(void*,void*,size_t*);
    const auto id=reinterpret_cast<ToString>(base+game_build::addresses.lua_to_string)(vm,lua_base,nullptr);
    log("MENU select=%.31s",id?id:"");
    WorkshopData selected{};
    const auto pool=mods_pool();
    for(unsigned i=0;i<std::min(pool->count,128u);i++)
        if(id && strcmp(id,pool->entries[i].publisher_id)==0) selected=pool->entries[i];
    {
        std::lock_guard lock(stack_lock);
        seed_locked();stack_initialized=true;
        if(is_loader(selected)) { /* The bootstrap is a control, not a gameplay slot. */ }
        else if(!valid(selected)) {if(!stack.empty())++hud_generation;stack.clear();}
        else {
            if(stack_logic::select(stack,selected,[](const auto& m){return std::string_view(m.publisher_id);}))++hud_generation;
        }
        log("STACK count=%zu",stack.size());
        for(const auto& m:stack) log("ENABLED %.31s %.99s",m.publisher_id,m.title);
    }
    return menu_set_mod(vm);
}
void assets_hook(ZoneInfo* zones,unsigned count,int sync,int suppress) {
    log("LOAD count=%u sync=%d suppress=%d",count,sync,suppress);
    for(unsigned i=0;i<count && i<128;i++) {
        const auto& z=zones[i];
        log("ZONE %.63s alloc=%x free=%x slot=%d free_slot=%d buffer=%p size=%zu",z.name?z.name:"<null>",z.alloc_flags,z.free_flags,z.alloc_slot,z.free_slot,z.buffer,z.buffer_size);
    }
    const auto selected=snapshot_stack();
    const auto has_zone=[](const auto& mod,const char* name) {
        std::error_code ec;
        return std::filesystem::is_regular_file(std::filesystem::path(mod.zone_directory)/(std::string(name)+".ff"),ec);
    };
    // Publish an immutable per-selection table before the engine loads or
    // unloads zones. Duplicate definitions obey ON 1, ON 2, ... across phases.
    priority_table.store(std::make_shared<const asset_priority::Table>(asset_priority::make_table(selected,has_zone,[](const auto& m){return m.publisher_id;})));
    priority_reports.store(0);
    stack_logic::ZonePlan plan({zones,count},selected,has_zone,[](const auto& m){return m.publisher_id;});
    // Each load batch supplies fresh source lists. Old unions stay alive only
    // for the engine's existing asset stashes, never as input to a new stack.
    {
        std::lock_guard lock(precache_lock);
        for(const auto& zone:plan.zones)if(zone.name && stack_logic::mod_zone(zone.name))
            std::erase_if(precache_sources,[&](const auto& item){return item.first.second==zone.name;});
    }
    if(std::any_of(plan.zones.begin(),plan.zones.end(),[](const auto& zone){return zone.name && !strcmp(zone.name,"zm_mod");})) {
        std::lock_guard lock(startup_script_lock);
        startup_scripts.clear();
    }
    for(const auto& mod:selected)remember_stream_provider(mod);
    log("PLAN requested=%u packages=%zu providers=%zu",count,plan.zones.size(),selected.size());
    load_assets(plan.zones.data(),static_cast<unsigned>(plan.zones.size()),sync,suppress);
}
int try_hook(const char* name,int flags,const void* buffer) {
    log("TRY %.63s flags=%x buffer=%p",name,flags,buffer);
    auto previous=current_extra;
    auto previous_zone=current_zone;
    current_zone=name;
    const auto selected=snapshot_stack();
    const auto alias=stack_logic::parse_alias(name,provider_ids(selected));
    current_extra=alias?alias->provider:"";
    std::string logical=name;
    if(!stack_logic::mod_zone(logical) && logical.size()>3 && logical[2]=='_')logical.erase(0,3);
    if(current_extra.empty() && stack_logic::mod_zone(logical))current_extra=provider_for(std::string(name)+".ff",selected);
    const auto result=try_load(name,flags,buffer);
    current_extra=previous;
    current_zone=previous_zone;
    log("RESULT %.63s result=%d",name,result);
    return result;
}

AssetEntry* asset_hook(AssetEntry* entry,int allow) {
    // Return to the game's load-error flow only after releasing our mutexes.
    try {combine_precache(entry);}
    catch(const std::exception& error){load_error(error.what());return nullptr;}
    if(entry->type==54 && entry->header && !current_extra.empty()) {
        struct Script {const char* name;uint32_t length,padding;const unsigned char* buffer;};
        auto script=static_cast<Script*>(entry->header);
        if(script->name && !strcmp(script->name,"scripts/zm/gametypes/_clientids.gsc") && script->buffer && script->length>=72 && script->length<1024*1024) {
            const auto selected=snapshot_stack();
            std::lock_guard lock(startup_script_lock);
            auto& captured=startup_scripts[current_extra];
            captured={{script->buffer,script->buffer+script->length},fingerprint(script->buffer,script->length)};
            log("STARTUP source provider=%s bytes=%u fingerprint=%llx",current_extra.c_str(),script->length,static_cast<unsigned long long>(captured.hash));
            // This pair's startup scripts have distinct responsibilities. Keep
            // Nacht's setup and prepend Ultimis's explicit character init.
            // The adapter is pinned to the two observed, unlinked scripts.
            const auto nacht=startup_scripts.find("3596966808"),ultimis=startup_scripts.find("3543204459");
            const auto enabled=[&](const char* id){return std::any_of(selected.begin(),selected.end(),[&](const auto& m){return !strcmp(m.publisher_id,id);});};
            if(enabled("3596966808") && enabled("3543204459") && nacht!=startup_scripts.end() && ultimis!=startup_scripts.end() &&
               nacht->second.hash==0xd3e6ed8abaec1983ull && ultimis->second.hash==0x48fc8eac176f53a2ull) {
                try {
                    auto merged=std::make_shared<std::vector<unsigned char>>(script_dependencies::nacht_ultimis(nacht->second.bytes,ultimis->second.bytes));
                    script->buffer=merged->data();script->length=static_cast<uint32_t>(merged->size());
                    retained_startup_scripts.push_back(std::move(merged));
                    log("STARTUP ADAPTER Nacht + Ultimis: Nacht setup and Ultimis character initialization retained (%u bytes)",script->length);
                } catch(const std::exception& error) {log("STARTUP ADAPTER refused: %s",error.what());}
            }
        }
    }
    if(!current_extra.empty() && entry->header &&
       ((entry->type>=23 && entry->type<=26) || entry->type==34 || entry->type==43 || entry->type==45 || entry->type==48)) {
        const auto name=*static_cast<const char**>(entry->header);
        if(name) {
            std::lock_guard lock(collision_lock);
            const auto key=std::make_pair(entry->type,std::string(name));
            if(const auto old=named_asset_providers.find(key);old!=named_asset_providers.end() && old->second!=current_extra)
                log("ASSET OVERLAP type=%d name=%s earlier=%s later=%s",entry->type,name,old->second.c_str(),current_extra.c_str());
            named_asset_providers[key]=current_extra;
            log("NAMED ASSET type=%d zone=%s name=%s",entry->type,current_zone.c_str(),name);
        }
    }
    if(!current_extra.empty() && entry->header && (entry->type==0x2f || entry->type==0x36)) {
        struct Bytes {const char* name;uint64_t length;const unsigned char* buffer;};
        const auto bytes=static_cast<const Bytes*>(entry->header);
        const size_t length=entry->type==0x36?static_cast<uint32_t>(bytes->length):bytes->length;
        if(bytes->name && bytes->buffer && length<16*1024*1024) {
            const auto hash=fingerprint(bytes->buffer,length);
            const auto key=std::make_pair(entry->type,std::string(bytes->name));
            std::lock_guard lock(collision_lock);
            if(const auto found=asset_versions.find(key);found!=asset_versions.end() && found->second.provider!=current_extra && found->second.hash!=hash) {
                const bool adapted=entry->type==0x2f && canonical(bytes->name)=="ui/hudinject/hudinject";
                log("%s type=%x name=%s earlier=%s later=%s",adapted?"HUD_SHARED":"CONFLICT",entry->type,bytes->name,found->second.provider.c_str(),current_extra.c_str());
            }
            asset_versions[key]={current_extra,hash};
        }
    }
    if(current_zone.find("_mod")!=std::string::npos && entry->type==0x2f && entry->header) {
        struct RawFile {const char* name; uint64_t length; const char* buffer;};
        auto raw=static_cast<RawFile*>(entry->header);
        log("RAW zone=%s name=%.240s length=%llu",current_zone.c_str(),raw->name?raw->name:"",raw->length);
        if(raw->name && canonical(raw->name)=="ui/hudinject/hudinject" && raw->buffer && raw->length>0 && raw->length<4*1024*1024) {
            std::string provider=current_extra;
            if(provider.empty())for(const auto& m:snapshot_stack()) {
                std::error_code ec;
                if(std::filesystem::is_regular_file(std::filesystem::path(m.zone_directory)/(current_zone+".ff"),ec)){provider=m.publisher_id;break;}
            }
            if(!provider.empty()) {
                auto copy=std::make_shared<HudModule>(HudModule{provider,std::string(raw->buffer,raw->length)});
                std::lock_guard lock(hud_lock);
                const auto old=hud_modules.find(provider);
                if(old==hud_modules.end() || old->second->bytes!=copy->bytes) {
                    hud_modules[provider]=copy;retained_hud_modules.push_back(copy);++hud_generation;
                    log("HUD captured provider=%s bytes=%zu",provider.c_str(),copy->bytes.size());
                }
            }
        }
    }
    if(current_zone.find("_mod")!=std::string::npos && entry->type==0x36 && entry->header) {
        const auto name=*static_cast<const char**>(entry->header);
        log("SCRIPT zone=%s name=%.240s",current_zone.c_str(),name?name:"");
    }
    // Asset headers can be swapped/stashed during linking. Track both pointers;
    // asynchronous sound opens receive the final bank object, not loader TLS.
    const auto sound=entry->type==10;
    const auto result=link_asset(entry,allow);
    // DB may swap a new header with an active one. Track both after that swap;
    // the diagnostic table is bounded by allocated script headers across reloads.
    remember_script(entry);
    if(result!=entry)remember_script(result);
    if(sound){remember_sound_owner(entry);if(result!=entry)remember_sound_owner(result);}
    return result;
}
int require_hook(void* vm) {
    const auto name=lua_runtime::argument(base,vm);
    if(name && std::string_view(name).starts_with("bo3stack/"))return ui_request(vm,std::string_view(name).substr(9));
    const auto path=name?canonical(name):std::string();
    if(path=="ui/t7/utility/codutilityrequire" || path=="ui/uieditor/menus/mods/menumodsload" || path=="ui/uieditor/menus/lobby/lobby") {
        const auto result=package_require(vm);
        std::string error;
        if(!lua_runtime::execute(base,vm,selection_editor::frontend_source,"bo3_stack/frontend",error))log("UI INSTALL ERROR %s",error.c_str());
        return result;
    }
    if(!name || canonical(name)!="ui/hudinject/hudinject")return package_require(vm);
    const auto selected=snapshot_stack();
    std::vector<std::shared_ptr<HudModule>> modules;
    {
        std::lock_guard lock(hud_lock);
        for(const auto& m:selected)if(const auto it=hud_modules.find(m.publisher_id);it!=hud_modules.end())modules.push_back(it->second);
    }
    if(modules.size()<2)return package_require(vm);
    const auto generation=hud_generation.load();
    const auto done=lua_runtime::global(vm,"BO3StackHudGeneration");
    float value{};memcpy(&value,&done.value,4);
    if((done.type&15)==3 && value==static_cast<float>(generation)) {
        *lua_runtime::top(vm)++=lua_runtime::Object{1,0,1};return 1;
    }
    log("HUD combining %zu module providers",modules.size());
    std::string error;
    const auto run=[&](std::string_view code,const char* chunk){return lua_runtime::execute(base,vm,code,chunk,error);};
    bool ok=run(R"lua(
rawset(_G, "BO3StackHudAssembly", {handlers={}, original=CoD.DemoUtility.AddHUDWidgets})
)lua","bo3_stack/hud_begin");
    for(const auto& mod:modules) {
        if(!ok)break;
        const auto chunk="bo3_stack/"+mod->provider+"/hudinject.lua";
        ok=run("CoD.DemoUtility.AddHUDWidgets=BO3StackHudAssembly.original","bo3_stack/hud_reset") && run(mod->bytes,chunk.c_str()) && run(R"lua(
local s=BO3StackHudAssembly
assert(type(CoD.DemoUtility.AddHUDWidgets)=="function", "HUD module did not provide a callback")
table.insert(s.handlers, {callback=CoD.DemoUtility.AddHUDWidgets, class=CoD.HUDInject})
)lua","bo3_stack/hud_capture");
        log("HUD provider=%s executed=%d",mod->provider.c_str(),ok);
    }
    if(ok)ok=run(R"lua(
local handlers=BO3StackHudAssembly.handlers
CoD.DemoUtility.AddHUDWidgets=function(hud, controller)
    for _, handler in ipairs(handlers) do
        local previous=CoD.HUDInject
        CoD.HUDInject=handler.class
        local ok, err=pcall(handler.callback, hud, controller)
        CoD.HUDInject=previous
        if not ok then error(err) end
    end
end
package.loaded["ui.hudinject.hudinject"]=true
rawset(_G, "BO3StackHudAssembly", nil)
)lua","bo3_stack/hud_finish");
    if(ok)ok=run("rawset(_G, 'BO3StackHudGeneration', "+std::to_string(generation)+")","bo3_stack/hud_generation");
    if(!ok){log("HUD ERROR %s",error.c_str());return package_require(vm);}
    *lua_runtime::top(vm)++=lua_runtime::Object{1,0,1};
    log("HUD combined successfully");return 1;
}
int path_hook(const char* name,const char* ext,int size,char* out,int type,const char* publisher) {
    if(!name || !ext || size<=0 || !out || type==2 || !stack_logic::safe_resource(std::string(name)+ext))return zone_path(name,ext,size,out,type,publisher);
    if(const auto stream=stack_logic::parse_stream_alias(std::string(name)+ext,stream_provider_ids())) {
        const auto folder=stream_folder(stream->provider);
        const int result=snprintf(out,size,"%s/%s",folder.c_str(),stream->resource.c_str());
        log("STREAM ROUTE %s%s => %s",name,ext,out);
        return result;
    }
    const auto selected=snapshot_stack();
    const auto alias=stack_logic::parse_alias(name,provider_ids(selected));
    const std::string actual=alias?alias->resource:name;
    std::string id=alias?alias->provider:(type==1?current_extra:"");
    if(id.empty() && type==1)id=provider_for(actual+ext,selected);
    if(!id.empty())if(const auto folder=stream_folder(id);!folder.empty()) {
        const int result=snprintf(out,size,"%s/%s%s",folder.c_str(),actual.c_str(),ext);
        log("ROUTE %.63s%.15s => %.259s",name,ext,out);
        return result;
    }
    for(const auto& m:selected) if(!id.empty() && id==m.publisher_id) {
        const int result=snprintf(out,size,"%s/%s%s",m.zone_directory,actual.c_str(),ext);
        log("ROUTE %.63s%.15s => %.259s",name,ext,out);
        return result;
    }
    const auto result=zone_path(name,ext,size,out,type,publisher);
    log("PATH %.63s%.15s type=%d publisher=%.31s => %.259s",name,ext,type,publisher?publisher:"",out);
    return result;
}

struct HookSpec { uintptr_t rva; const char* bytes; size_t length; void* hook; void** original; };
DWORD WINAPI start(void*) {
    // Keep the handles alive for the process lifetime so repeated launches can
    // distinguish ready from a DLL that loaded but failed to initialize.
    static stack_status::Signals signals(GetCurrentProcessId());
    static stack_status::SharedDetails details(GetCurrentProcessId());
    struct ReportFailure { bool ready=false;~ReportFailure(){if(!ready && signals.failed)SetEvent(signals.failed);} } report;
    const auto fail=[&](LONG code,const std::string& message){details.publish(code,message);log("REFUSED code=%ld %s",code,message.c_str());return static_cast<DWORD>(code);};
    if(!signals)return fail(12,"Windows could not create startup signals. Restart BO3 and try again.");
    wchar_t path[32768]; GetModuleFileNameW(module,path,32768);
    const auto log_path=std::filesystem::path(path).parent_path()/L"bo3_stack.log";
    logfile=_wfsopen(log_path.c_str(),L"a",_SH_DENYNO);
    if(!logfile)return fail(13,"Cannot write bo3_stack.log in the loader's installed folder. Check that this folder is writable, then restart BO3.");
    base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    wchar_t game_path[32768]; GetModuleFileNameW(nullptr,game_path,32768);
    log("VERSION %s",stack_status::version);
    const auto observed_hash=build_identity::sha256(game_path);
    const auto profile=game_build::find(observed_hash);
    log("BUILD SHA-256=%s profile=%s",observed_hash.c_str(),profile?profile->steam_build:"unsupported");
    if(!profile)return fail(1,game_build::unsupported_message(observed_hash));
    const auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || dos->e_lfanew>0x800)
        return fail(1,"The loaded executable DOS header is invalid. No game patches were applied.");
    const auto pe=reinterpret_cast<IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    log("START stack extension base=%p checksum=%x",reinterpret_cast<void*>(base),pe->OptionalHeader.CheckSum);
    if(pe->Signature!=IMAGE_NT_SIGNATURE || pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 ||
       pe->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
       !game_build::matches_image(*profile,pe->OptionalHeader.CheckSum,pe->OptionalHeader.SizeOfImage))
        return fail(1,"The loaded executable header does not match its supported Steam build profile. No game patches were applied. Restart BO3 using the supported executable.");
    game_build::addresses=profile->addresses;
    asset_zone_name=reinterpret_cast<AssetZoneName>(base+game_build::addresses.asset_zone_name);
    // Validate the extra native calls as well as all detour entry points.
    const std::array reload_calls{
        HookSpec{game_build::addresses.load_mod_by_id,"\x48\x89\x5c\x24\x18\x55\x56\x57\x41\x56\x41\x57",12,nullptr,nullptr},
        HookSpec{game_build::addresses.mod_assets_busy,"\x48\x89\x5c\x24\x08\x57\x48\x83\xec\x20",10,nullptr,nullptr},
        HookSpec{game_build::addresses.asset_zone_name,"\x0f\xb6\x41\x10\x84\xc0",6,nullptr,nullptr},
        HookSpec{game_build::addresses.lua_to_string,"\x40\x53\x57\x41\x56\x48\x81\xec\x50\x01\x00\x00",12,nullptr,nullptr},
        HookSpec{game_build::addresses.lua_load_buffer,"\x40\x53\x48\x81\xec\x70\x01\x00\x00",9,nullptr,nullptr},
        HookSpec{game_build::addresses.lua_pcall,"\x48\x89\x5c\x24\x08\x48\x89\x6c\x24\x18",10,nullptr,nullptr},
        HookSpec{game_build::addresses.game_error,"\x4c\x89\x4c\x24\x20\x55\x53\x56\x57\x41\x54",11,nullptr,nullptr}
    };
    for(const auto& entry:reload_calls)if(memcmp(reinterpret_cast<void*>(base+entry.rva),entry.bytes,entry.length)!=0) {
        log("REFUSED native call bytes differ rva=%llx",static_cast<unsigned long long>(entry.rva));
        return fail(2,"Another native patch changed a native function needed by Workshop Stack. See bo3_stack.log and restart BO3 after changing the patch setup.");
    }
    load_mod_by_id=reinterpret_cast<LoadModById>(base+game_build::addresses.load_mod_by_id);
    mod_assets_busy=reinterpret_cast<bool(*)()>(base+game_build::addresses.mod_assets_busy);
    mod_load_state=reinterpret_cast<const int*>(base+game_build::addresses.mod_load_state);
    const std::array hooks{
        HookSpec{game_build::addresses.menu_set_mod,"\x48\x83\xec\x28\x48\x8b\x51\x50",8,reinterpret_cast<void*>(&menu_mod_hook),reinterpret_cast<void**>(&menu_set_mod)},
        HookSpec{game_build::addresses.check_mod_changes,"\x48\x83\xec\x28\x85\xc9\x75\x78",8,reinterpret_cast<void*>(&mod_change_check_hook),reinterpret_cast<void**>(&check_mod_changes)},
        HookSpec{game_build::addresses.load_assets,"\x4c\x8b\xdc\x56\x48\x81\xec\x60\x0d\x00\x00",11,reinterpret_cast<void*>(&assets_hook),reinterpret_cast<void**>(&load_assets)},
        HookSpec{game_build::addresses.try_load,"\x48\x89\x5c\x24\x20\x55\x56\x57",8,reinterpret_cast<void*>(&try_hook),reinterpret_cast<void**>(&try_load)},
        HookSpec{game_build::addresses.zone_path,"\x48\x89\x6c\x24\x10\x48\x89\x7c\x24\x18",10,reinterpret_cast<void*>(&path_hook),reinterpret_cast<void**>(&zone_path)},
        HookSpec{game_build::addresses.menu_info,"\x48\x89\x5c\x24\x10\x48\x89\x6c\x24\x18",10,reinterpret_cast<void*>(&info_hook),reinterpret_cast<void**>(&menu_info)},
        HookSpec{game_build::addresses.draw_mod_label,"\x48\x89\x5c\x24\x20\x56\x48\x81\xec\x90\x00\x00\x00",13,reinterpret_cast<void*>(&mod_label_hook),reinterpret_cast<void**>(&draw_mod_label)},
        HookSpec{game_build::addresses.link_asset,"\x48\x89\x5c\x24\x18\x89\x54\x24\x10",9,reinterpret_cast<void*>(&asset_hook),reinterpret_cast<void**>(&link_asset)},
        HookSpec{game_build::addresses.asset_priority,"\x48\x89\x5c\x24\x08\x48\x89\x6c\x24\x10",10,reinterpret_cast<void*>(&priority_hook),reinterpret_cast<void**>(&native_asset_priority)},
        HookSpec{game_build::addresses.package_require,"\x4c\x8b\xdc\x55\x53\x56\x41\x55\x41\x57",10,reinterpret_cast<void*>(&require_hook),reinterpret_cast<void**>(&package_require)},
        HookSpec{game_build::addresses.allocate_asset,"\x40\x57\x48\x83\xec\x30",6,reinterpret_cast<void*>(&allocate_hook),reinterpret_cast<void**>(&allocate_asset)},
        HookSpec{game_build::addresses.prepare_zones,"\x4c\x89\x44\x24\x18\x55\x53\x57",8,reinterpret_cast<void*>(&prepare_hook),reinterpret_cast<void**>(&prepare_zones)},
        HookSpec{game_build::addresses.schedule_zones,"\x48\x89\x5c\x24\x18\x48\x89\x6c\x24\x20",10,reinterpret_cast<void*>(&schedule_hook),reinterpret_cast<void**>(&schedule_zones)}
        ,HookSpec{game_build::addresses.zone_dependency,"\x48\x89\x5c\x24\x08\x48\x89\x6c\x24\x10",10,reinterpret_cast<void*>(&dependency_hook),reinterpret_cast<void**>(&zone_dependency)}
        ,HookSpec{game_build::addresses.open_sound_bank,"\x48\x89\x5c\x24\x20\x55\x56\x57",8,reinterpret_cast<void*>(&sound_bank_hook),reinterpret_cast<void**>(&open_sound_bank)}
        ,HookSpec{game_build::addresses.script_error,"\x48\x89\x5c\x24\x10",5,reinterpret_cast<void*>(&script_error_hook),reinterpret_cast<void**>(&script_error)}
    };
    const auto& active_hooks=hooks;
    for(const auto& h:active_hooks) if(memcmp(reinterpret_cast<void*>(base+h.rva),h.bytes,h.length)!=0) {
        log("REFUSED function bytes differ rva=%llx",static_cast<unsigned long long>(h.rva));
        return fail(2,"Another native patch has changed a game function needed by Workshop Stack. This combination is not supported. See bo3_stack.log for the conflicting function. Restart BO3 after changing the patch setup.");
    }
    {std::lock_guard lock(stack_lock);seed_locked();}
    if(!native_compat::apply(base,profile->sites)) {
        const auto& result=native_compat::transaction;
        log("COMPAT refused error=%d rva=%x rollback_ok=%d",static_cast<int>(result.error),result.failed_rva,result.rollback_ok);
        return fail(6,"The game's native patch state could not be verified. Only original or recognized compatible patch bytes are accepted. See bo3_stack.log for details, then restart BO3. If using T7, let it finish loading before activating Workshop Stack.");
    }
    log("COMPAT verified %zu sites; applied=%zu reused=%zu",profile->sites.size(),native_compat::transaction.applied,native_compat::transaction.reused);
    const auto rollback=[&](){const bool ok=native_compat::restore(base,profile->sites);log("COMPAT rollback owned changes only: %s",ok?"ok":"incomplete; restart required");};
    if(MH_Initialize()!=MH_OK){rollback();return fail(9,"The native hook library could not initialize. Restart BO3 before retrying.");}
    for(const auto& h:active_hooks) {
        const auto status=MH_CreateHook(reinterpret_cast<void*>(base+h.rva),h.hook,h.original);
        if(status!=MH_OK){log("ERROR native hook %s",MH_StatusToString(status));MH_Uninitialize();rollback();return fail(10,"A native game hook could not be created. See bo3_stack.log for the hook error, then restart BO3.");}
    }
    if(MH_EnableHook(MH_ALL_HOOKS)!=MH_OK){MH_DisableHook(MH_ALL_HOOKS);MH_Uninitialize();rollback();return fail(11,"A native game hook could not be activated. Restart BO3 before retrying.");}
    if(!t7_adapter.start()) {
        MH_DisableHook(MH_ALL_HOOKS);MH_Uninitialize();rollback();
        return fail(14,"Windows could not register native-module compatibility notifications. Restart BO3 before retrying.");
    }
    report_t7();
    log("HOOKS ready with native compatibility; T7 load notifications active");
    details.publish(1,"BO3 Workshop Stack is ready.");
    report.ready=true;SetEvent(signals.ready);
    return 0;
}
}
// HavokScript's package.loadlib calls this only after the user's mod activation.
// Return no values on failure; the Lua bootstrap reports it without touching an
// unverified VM layout. The existing EXE can still use the ready/failed events.
extern "C" __declspec(dllexport) int BO3Stack_Initialize(void* vm) {
    stack_status::Signals signals(GetCurrentProcessId());
    stack_status::SharedDetails details(GetCurrentProcessId());
    const HANDLE events[]{signals.ready,signals.failed};
    const auto result=signals?WaitForMultipleObjects(2,events,FALSE,15000):WAIT_FAILED;
    if(result!=WAIT_OBJECT_0) {
        const auto message=result==WAIT_OBJECT_0+1?details.message():std::string();
        const auto fallback=result==WAIT_TIMEOUT?"Startup timed out. Restart BO3; if using T7, wait until it is fully loaded before activating Workshop Stack. See bo3_stack.log in this mod's folder.":"Startup failed before diagnostic details were available. Restart BO3 and check bo3_stack.log in this mod's folder.";
        // A Windows dialog works even on a rejected/downpatched executable,
        // where calling hard-coded Lua functions or writing VM objects is unsafe.
        MessageBoxA(nullptr,message.empty()?fallback:message.c_str(),"BO3 Workshop Stack - startup failed",MB_OK|MB_ICONERROR|MB_TASKMODAL);
        return 0;
    }
    *lua_runtime::top(vm)++=lua_runtime::Object{1,0,1};
    return 1;
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        module=instance; DisableThreadLibraryCalls(instance);
        const auto thread=CreateThread(nullptr,0,start,nullptr,0,nullptr);
        if(thread) CloseHandle(thread);
    }
    return TRUE;
}
