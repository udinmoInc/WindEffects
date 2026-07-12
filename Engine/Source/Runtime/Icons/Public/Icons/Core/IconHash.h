#pragma once

#include "Icons/Export.h"

#include <cstdint>
#include <string_view>

namespace we::runtime::icons {

constexpr uint64_t kFnv1aOffsetBasis = 14695981039346656037ULL;
constexpr uint64_t kFnv1aPrime = 1099511628211ULL;

[[nodiscard]] constexpr uint64_t Fnv1a64(std::string_view text)
{
    uint64_t hash = kFnv1aOffsetBasis;
    for (const unsigned char c : text) {
        hash ^= static_cast<uint64_t>(c);
        hash *= kFnv1aPrime;
    }
    return hash;
}

[[nodiscard]] inline uint64_t MakeIconLookupKey(uint64_t nameHash, uint32_t tierPx)
{
    return nameHash ^ (static_cast<uint64_t>(tierPx) * 0x9E3779B97F4A7C15ULL);
}

} // namespace we::runtime::icons
