#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include "game_build.hpp"

namespace build_identity {
inline std::string sha256(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary); if(!file)return {};
    BCRYPT_ALG_HANDLE algorithm{}; BCRYPT_HASH_HANDLE hash{};
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return {};
    if(BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)<0){BCryptCloseAlgorithmProvider(algorithm,0);return {};}
    std::array<char,65536> buf; bool ok=true;
    while(file.read(buf.data(),buf.size()) || file.gcount())
        if(BCryptHashData(hash,reinterpret_cast<PUCHAR>(buf.data()),static_cast<ULONG>(file.gcount()),0)<0){ok=false;break;}
    std::array<UCHAR,32> digest{};
    ok=ok && !file.bad() && BCryptFinishHash(hash,digest.data(),32,0)>=0;
    BCryptDestroyHash(hash); BCryptCloseAlgorithmProvider(algorithm,0);
    if(!ok)return {};
    const char* digits="0123456789abcdef"; std::string result;
    for(auto b:digest){result+=digits[b>>4];result+=digits[b&15];}
    return result;
}
}
