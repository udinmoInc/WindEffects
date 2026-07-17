#include "WindEffects/Runtime/UI/Rendering/Icons/SvgRasterizer.h"
#include "Core/Logger.h"

#include <lunasvg.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace WindEffects::Editor::UI::Icons {

namespace {

std::string LoadSvgText(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void ReplaceAll(std::string& text, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return;
    }
    size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}

std::string PrepareSvgForRasterization(const std::string& svgText) {
    std::string prepared = svgText;
    ReplaceAll(prepared, "currentColor", "#000000");
    return prepared;
}

} // namespace

std::string SvgRasterizer::ResolveAssetPath(const std::string& relativePath) {
    if (relativePath.empty()) {
        return {};
    }
    if (std::filesystem::exists(relativePath)) {
        return relativePath;
    }

    const std::string candidates[] = {
        "../" + relativePath,
        "../../" + relativePath,
        "../../../" + relativePath,
        "Engine/Content/" + relativePath,
        "../Engine/Content/" + relativePath,
    };
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return relativePath;
}

SvgRasterizeResult SvgRasterizer::Rasterize(const SvgRasterizeRequest& request) const {
    SvgRasterizeResult result{};
    if (request.width == 0 || request.height == 0) {
        return result;
    }

    const std::string resolvedPath = ResolveAssetPath(request.svgPath);
    const std::string svgText = LoadSvgText(resolvedPath);
    if (svgText.empty()) {
        HE_ERROR("[UI] LunaSVG failed to read: " + resolvedPath);
        return result;
    }

    const std::string preparedSvg = PrepareSvgForRasterization(svgText);
    auto document = lunasvg::Document::loadFromData(preparedSvg.c_str(), preparedSvg.size());
    if (!document) {
        HE_ERROR("[UI] LunaSVG failed to load: " + resolvedPath);
        return result;
    }

    lunasvg::Bitmap bitmap = document->renderToBitmap(
        static_cast<int>(request.width),
        static_cast<int>(request.height));
    if (!bitmap.valid()) {
        HE_ERROR("[UI] LunaSVG failed to rasterize: " + resolvedPath);
        return result;
    }

    bitmap.convertToRGBA();
    const uint8_t* src = bitmap.data();
    if (!src) {
        return result;
    }

    const uint32_t srcWidth = bitmap.width();
    const uint32_t srcHeight = bitmap.height();
    const uint32_t stride = bitmap.stride();
    if (srcWidth == 0 || srcHeight == 0 || stride < srcWidth * 4) {
        return result;
    }

    const size_t pixelCount = static_cast<size_t>(request.width) * static_cast<size_t>(request.height);
    result.rgba.resize(pixelCount * 4);
    result.width = request.width;
    result.height = request.height;

    for (uint32_t y = 0; y < request.height; ++y) {
        for (uint32_t x = 0; x < request.width; ++x) {
            const size_t srcIdx = static_cast<size_t>(y) * stride + static_cast<size_t>(x) * 4;
            const size_t dstIdx = (static_cast<size_t>(y) * request.width + x) * 4;
            const uint8_t r = src[srcIdx + 0];
            const uint8_t g = src[srcIdx + 1];
            const uint8_t b = src[srcIdx + 2];
            const uint8_t a = src[srcIdx + 3];

            if (request.applyTint) {
                const float mask = a / 255.0f;
                result.rgba[dstIdx + 0] = static_cast<uint8_t>(request.tint.r * 255.0f);
                result.rgba[dstIdx + 1] = static_cast<uint8_t>(request.tint.g * 255.0f);
                result.rgba[dstIdx + 2] = static_cast<uint8_t>(request.tint.b * 255.0f);
                result.rgba[dstIdx + 3] = static_cast<uint8_t>(mask * request.tint.a * 255.0f);
            } else {
                result.rgba[dstIdx + 0] = r;
                result.rgba[dstIdx + 1] = g;
                result.rgba[dstIdx + 2] = b;
                result.rgba[dstIdx + 3] = a;
            }
        }
    }

    result.success = true;
    return result;
}

} // namespace WindEffects::Editor::UI::Icons
