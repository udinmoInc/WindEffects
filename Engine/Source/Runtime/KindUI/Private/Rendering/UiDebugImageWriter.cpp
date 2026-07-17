#include "Rendering/UiDebugImageWriter.h"

#include "Core/Logger.h"

#include <cstdio>
#include <filesystem>
#include <vector>

namespace we::runtime::kindui {

bool SaveBmpRgba(const std::string& path, const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height) {
    if (path.empty() || rgba.empty() || width == 0 || height == 0) {
        return false;
    }
    if (rgba.size() < static_cast<size_t>(width) * static_cast<size_t>(height) * 4) {
        return false;
    }

    std::error_code ec;
    const std::filesystem::path outPath(path);
    if (outPath.has_parent_path()) {
        std::filesystem::create_directories(outPath.parent_path(), ec);
    }

    FILE* file = nullptr;
#if defined(_MSC_VER)
    if (fopen_s(&file, path.c_str(), "wb") != 0 || !file) {
        return false;
    }
#else
    file = std::fopen(path.c_str(), "wb");
    if (!file) {
        return false;
    }
#endif

    const uint32_t rowBytes = width * 4;
    const uint32_t imageSize = rowBytes * height;
    const uint32_t fileSize = 14 + 40 + imageSize;

    uint8_t fileHeader[14] = {
        'B', 'M',
        static_cast<uint8_t>(fileSize), static_cast<uint8_t>(fileSize >> 8),
        static_cast<uint8_t>(fileSize >> 16), static_cast<uint8_t>(fileSize >> 24),
        0, 0, 0, 0,
        54, 0, 0, 0,
    };

    uint8_t infoHeader[40] = {};
    infoHeader[0] = 40;
    infoHeader[4] = static_cast<uint8_t>(width);
    infoHeader[5] = static_cast<uint8_t>(width >> 8);
    infoHeader[6] = static_cast<uint8_t>(width >> 16);
    infoHeader[7] = static_cast<uint8_t>(width >> 24);
    infoHeader[8] = static_cast<uint8_t>(height);
    infoHeader[9] = static_cast<uint8_t>(height >> 8);
    infoHeader[10] = static_cast<uint8_t>(height >> 16);
    infoHeader[11] = static_cast<uint8_t>(height >> 24);
    infoHeader[12] = 1;
    infoHeader[14] = 32;
    infoHeader[20] = static_cast<uint8_t>(imageSize);
    infoHeader[21] = static_cast<uint8_t>(imageSize >> 8);
    infoHeader[22] = static_cast<uint8_t>(imageSize >> 16);
    infoHeader[23] = static_cast<uint8_t>(imageSize >> 24);

    std::fwrite(fileHeader, 1, sizeof(fileHeader), file);
    std::fwrite(infoHeader, 1, sizeof(infoHeader), file);

    std::vector<uint8_t> row(rowBytes);
    for (int32_t y = static_cast<int32_t>(height) - 1; y >= 0; --y) {
        const size_t srcRow = static_cast<size_t>(y) * static_cast<size_t>(width) * 4;
        for (uint32_t x = 0; x < width; ++x) {
            const size_t src = srcRow + static_cast<size_t>(x) * 4;
            const size_t dst = static_cast<size_t>(x) * 4;
            row[dst + 0] = rgba[src + 2];
            row[dst + 1] = rgba[src + 1];
            row[dst + 2] = rgba[src + 0];
            row[dst + 3] = rgba[src + 3];
        }
        std::fwrite(row.data(), 1, rowBytes, file);
    }

    std::fclose(file);
    HE_INFO("[UI AtlasDebug] Saved debug image: " + path);
    return true;
}

} // namespace we::runtime::kindui
