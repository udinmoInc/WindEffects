#pragma once

#include "Icons/Export.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace we::runtime::icons::compiling::detail {

struct ParsedAtlasRegion {
    std::string sourceName;
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t width = 0;
    uint16_t height = 0;
};

struct ParsedAtlasDescriptor {
    std::filesystem::path pngPath;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t tierPx = 0;
    std::vector<ParsedAtlasRegion> regions;
};

[[nodiscard]] bool ParseLibGdxAtlasFile(
    const std::filesystem::path& atlasPath,
    const std::filesystem::path& searchRoot,
    ParsedAtlasDescriptor& outDescriptor,
    std::string& outError);

[[nodiscard]] bool LoadPngRgbaNative(
    const std::filesystem::path& path,
    std::vector<uint8_t>& outRgba,
    uint32_t& outWidth,
    uint32_t& outHeight);

[[nodiscard]] std::string ResolveRuntimeIconName(const std::string& atlasRegionName);
[[nodiscard]] bool IsFullColorIcon(const std::string& runtimeName);
[[nodiscard]] std::vector<std::string> RuntimeAliasesFor(const std::string& runtimeName);

} // namespace we::runtime::icons::compiling::detail
