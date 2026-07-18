#include "AssetImporter/AssetGuid.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>
#include <sstream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")
#endif

namespace we::runtime::assetimporter {
namespace {

void FillRandomBytes(uint8_t* data, size_t count) {
#if defined(_WIN32)
    HCRYPTPROV provider = 0;
    if (CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        const BOOL ok = CryptGenRandom(provider, static_cast<DWORD>(count), data);
        CryptReleaseContext(provider, 0);
        if (ok) {
            return;
        }
    }
#endif
    thread_local std::mt19937_64 rng{
        static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count())
        ^ static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&rng))
    };
    for (size_t i = 0; i < count; ++i) {
        data[i] = static_cast<uint8_t>(rng() & 0xFFu);
    }
}

int HexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

} // namespace

AssetGuid AssetGuid::Generate() {
    AssetGuid guid{};
    FillRandomBytes(guid.bytes.data(), guid.bytes.size());
    // RFC 4122 UUID version 4.
    guid.bytes[6] = static_cast<uint8_t>((guid.bytes[6] & 0x0F) | 0x40);
    guid.bytes[8] = static_cast<uint8_t>((guid.bytes[8] & 0x3F) | 0x80);
    return guid;
}

AssetGuid AssetGuid::FromBytes(const std::array<uint8_t, 16>& value) {
    AssetGuid guid{};
    guid.bytes = value;
    return guid;
}

std::optional<AssetGuid> AssetGuid::Parse(std::string_view text) {
    std::string hex;
    hex.reserve(32);
    for (char c : text) {
        if (c == '-' || c == '{' || c == '}' || c == '(' || c == ')') {
            continue;
        }
        if (HexValue(c) < 0) {
            return std::nullopt;
        }
        hex.push_back(c);
    }
    if (hex.size() != 32) {
        return std::nullopt;
    }

    AssetGuid guid{};
    for (size_t i = 0; i < 16; ++i) {
        const int hi = HexValue(hex[i * 2]);
        const int lo = HexValue(hex[i * 2 + 1]);
        guid.bytes[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    return guid;
}

bool AssetGuid::IsNil() const noexcept {
    for (uint8_t b : bytes) {
        if (b != 0) {
            return false;
        }
    }
    return true;
}

std::string AssetGuid::ToString() const {
    char buffer[37]{};
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0], bytes[1], bytes[2], bytes[3],
        bytes[4], bytes[5],
        bytes[6], bytes[7],
        bytes[8], bytes[9],
        bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    return std::string(buffer);
}

bool AssetGuid::operator==(const AssetGuid& other) const noexcept {
    return bytes == other.bytes;
}

std::size_t AssetGuidHash::operator()(const AssetGuid& guid) const noexcept {
    std::size_t hash = 1469598103934665603ull;
    for (uint8_t b : guid.bytes) {
        hash ^= b;
        hash *= 1099511628211ull;
    }
    return hash;
}

} // namespace we::runtime::assetimporter
