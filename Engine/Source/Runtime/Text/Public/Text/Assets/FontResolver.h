#pragma once

#include "Text/Assets/FontAssetManager.h"
#include "Text/Export.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::text::assets {

struct FontFaceRequest {
    std::string family = "Inter";
    uint16_t weight = 400;
    bool italic = false;
};

class TEXT_API IFontResolver {
public:
    virtual ~IFontResolver() = default;
    [[nodiscard]] virtual FontHandle Resolve(const FontFaceRequest& request) const = 0;
    virtual void RegisterFace(
        FontHandle handle,
        std::string family,
        uint16_t weight,
        bool italic,
        std::filesystem::path sourcePath = {}) = 0;
    [[nodiscard]] virtual std::optional<std::filesystem::path> SourcePathFor(FontHandle handle) const = 0;
};

[[nodiscard]] TEXT_API std::unique_ptr<IFontResolver> CreateFontResolver(IFontAssetManager& assets);

} // namespace we::runtime::text::assets
