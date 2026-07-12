#pragma once

#include "Icons/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::icons {

constexpr uint32_t kIconAtlasTiers[] = {16, 20, 24, 32, 48, 64};
constexpr uint32_t kIconAtlasTierCount = 6;

enum class IconAtlasFormat : uint8_t {
    Rgba8 = 0,
};

enum class IconEntryFlags : uint8_t {
    None = 0,
    FullColor = 1 << 0,
};

inline IconEntryFlags operator|(IconEntryFlags lhs, IconEntryFlags rhs)
{
    return static_cast<IconEntryFlags>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline bool HasFlag(IconEntryFlags flags, IconEntryFlags test)
{
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(test)) != 0;
}

struct IconUvRect {
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
};

struct IconPixelRect {
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t width = 0;
    uint16_t height = 0;
};

struct IconAtlasPage {
    uint32_t tierPx = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    IconAtlasFormat format = IconAtlasFormat::Rgba8;
    std::vector<uint8_t> rgba;
};

struct IconMetaEntry {
    uint64_t nameHash = 0;
    std::string name;
    uint32_t tierPx = 0;
    IconUvRect uv{};
    IconPixelRect pixel{};
    IconEntryFlags flags = IconEntryFlags::None;
};

struct IconMetaAsset {
    uint32_t version = 0;
    std::vector<IconMetaEntry> entries;
};

struct IconAtlasAsset {
    uint32_t version = 0;
    IconAtlasPage page{};
};

} // namespace we::runtime::icons
