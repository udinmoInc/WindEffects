// ==============================================================================
// WindEffects — Icons — ImageDecodeBackend
// Internal implementation for the Icons module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma warning(push)
#pragma warning(disable : 4505)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma warning(pop)

#include "Icons/Compile/IconCompileDetail.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace we::runtime::icons::compiling::detail {

bool LoadPngRgbaNative(
    const std::filesystem::path& path,
    std::vector<uint8_t>& outRgba,
    uint32_t& outWidth,
    uint32_t& outHeight)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels || width <= 0 || height <= 0) {
        if (pixels) {
            stbi_image_free(pixels);
        }
        return false;
    }

    outWidth = static_cast<uint32_t>(width);
    outHeight = static_cast<uint32_t>(height);
    const size_t byteCount = static_cast<size_t>(outWidth) * static_cast<size_t>(outHeight) * 4u;
    outRgba.assign(pixels, pixels + byteCount);
    stbi_image_free(pixels);
    return true;
}

} // namespace we::runtime::icons::compiling::detail
