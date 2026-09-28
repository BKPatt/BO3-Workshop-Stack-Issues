#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace precache {
// Verified against live BG_CACHE headers and records on the supported build.
// Names are inline; copying a list never retains pointers into another zone.
struct Record { uint32_t type; char name[128]; };
struct Header {const char* name;const Record* records;uint32_t count,padding;};
static_assert(sizeof(Record)==132 && sizeof(Header)==24);
using Records=std::vector<Record>;
inline Records copy(const Header& header) {
    if(header.count>1024*1024 || (header.count && !header.records))throw std::runtime_error("Invalid mod precache list");
    Records result;
    if(header.count)result.assign(header.records,header.records+header.count);
    for(const auto& record:result)
        if(record.type==0 || record.type>=29 || !memchr(record.name,0,sizeof(record.name)))
            throw std::runtime_error("Unsupported mod precache record");
    return result;
}
inline Records combine(const std::vector<const Records*>& sources) {
    // Native lists are ordered by type and name. Preserve that order and remove
    // exact duplicate requests, while allowing one name in multiple categories.
    std::map<std::pair<uint32_t,std::string>,Record> unique;
    for(const auto source:sources)for(const auto& record:*source)unique.try_emplace({record.type,record.name},record);
    if(unique.size()>UINT32_MAX)throw std::runtime_error("Combined mod precache list is too large");
    Records result;result.reserve(unique.size());
    for(const auto& [key,record]:unique)result.push_back(record);
    return result;
}
}
