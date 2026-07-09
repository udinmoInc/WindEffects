#include "Rendering/Icons/SvgRasterizer.h"
#include "Core/Logger.h"

#include <lunasvg.h>

#include <algorithm>
#include <filesystem>

namespace we::UI::Icons {

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
    auto document = lunasvg::Document::loadFromFile(resolvedPath);
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

    const size_t pixelCount = static_cast<size_t>(request.width) * static_cast<size_t>(request.height);
    result.rgba.resize(pixelCount * 4);
    result.width = request.width;
    result.height = request.height;

    for (size_t i = 0; i < pixelCount; ++i) {
        const size_t idx = i * 4;
        const uint8_t r = src[idx + 0];
        const uint8_t g = src[idx + 1];
        const uint8_t b = src[idx + 2];
        const uint8_t a = src[idx + 3];

        if (request.applyTint) {
            const float mask = a / 255.0f;
            result.rgba[i * 4 + 0] = static_cast<uint8_t>(request.tint.r * 255.0f);
            result.rgba[i * 4 + 1] = static_cast<uint8_t>(request.tint.g * 255.0f);
            result.rgba[i * 4 + 2] = static_cast<uint8_t>(request.tint.b * 255.0f);
            result.rgba[i * 4 + 3] = static_cast<uint8_t>(mask * request.tint.a * 255.0f);
        } else {
            result.rgba[i * 4 + 0] = r;
            result.rgba[i * 4 + 1] = g;
            result.rgba[i * 4 + 2] = b;
            result.rgba[i * 4 + 3] = a;
        }
    }

    result.success = true;
    return result;
}

} // namespace we::UI::Icons
