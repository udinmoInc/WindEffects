#include "Text/Assets/FontResolver.h"

#include <algorithm>

namespace we::runtime::text::assets {
namespace {

struct FaceRecord {
    FontHandle handle = kInvalidFontHandle;
    std::string family;
    uint16_t weight = 400;
    bool italic = false;
    std::filesystem::path sourcePath;
};

class FontResolver final : public IFontResolver {
public:
    explicit FontResolver(IFontAssetManager& assets)
        : m_Assets(assets)
    {
    }

    FontHandle Resolve(const FontFaceRequest& request) const override
    {
        FontHandle best = kInvalidFontHandle;
        int bestScore = -1;
        for (const auto& face : m_Faces) {
            if (face.family != request.family && face.family.find(request.family) == std::string::npos) {
                continue;
            }
            int score = 1000 - std::abs(static_cast<int>(face.weight) - static_cast<int>(request.weight));
            if (face.italic == request.italic) {
                score += 50;
            }
            if (score > bestScore) {
                bestScore = score;
                best = face.handle;
            }
        }
        if (best != kInvalidFontHandle) {
            return best;
        }
        const auto loaded = m_Assets.GetLoadedFonts();
        return loaded.empty() ? kInvalidFontHandle : loaded.front();
    }

    void RegisterFace(
        const FontHandle handle,
        std::string family,
        const uint16_t weight,
        const bool italic,
        std::filesystem::path sourcePath) override
    {
        for (auto& face : m_Faces) {
            if (face.handle == handle) {
                face.family = std::move(family);
                face.weight = weight;
                face.italic = italic;
                face.sourcePath = std::move(sourcePath);
                return;
            }
        }
        m_Faces.push_back(FaceRecord{handle, std::move(family), weight, italic, std::move(sourcePath)});
    }

    std::optional<std::filesystem::path> SourcePathFor(const FontHandle handle) const override
    {
        for (const auto& face : m_Faces) {
            if (face.handle == handle && !face.sourcePath.empty()) {
                return face.sourcePath;
            }
        }
        if (const auto asset = m_Assets.GetAsset(handle); asset && !asset->sourcePath.empty()) {
            return asset->sourcePath;
        }
        return std::nullopt;
    }

private:
    IFontAssetManager& m_Assets;
    mutable std::vector<FaceRecord> m_Faces;
};

} // namespace

std::unique_ptr<IFontResolver> CreateFontResolver(IFontAssetManager& assets)
{
    return std::make_unique<FontResolver>(assets);
}

} // namespace we::runtime::text::assets
