#include "Controllers/FilterController.h"
#include "Registry/ContentAssetRegistry.h"
#include <chrono>

namespace we::editor::contentbrowser {

void FilterController::ToggleFilter(ContentFilter filter) {
    const uint32_t bit = static_cast<uint32_t>(filter);
    if ((static_cast<uint32_t>(m_ActiveFilters) & bit) != 0) {
        m_ActiveFilters = static_cast<ContentFilter>(static_cast<uint32_t>(m_ActiveFilters) & ~bit);
    } else {
        m_ActiveFilters = static_cast<ContentFilter>(static_cast<uint32_t>(m_ActiveFilters) | bit);
    }
}

bool FilterController::MatchesTypeFilter(const AssetRecord& asset, ContentFilter filters) {
    if (asset.isFolder) return HasFilter(filters, ContentFilter::Folders);

    if (HasFilter(filters, ContentFilter::Textures) && asset.type == AssetType::Texture) return true;
    if (HasFilter(filters, ContentFilter::Materials) &&
        (asset.type == AssetType::Material || asset.type == AssetType::MaterialInstance)) return true;
    if (HasFilter(filters, ContentFilter::Meshes) &&
        (asset.type == AssetType::StaticMesh || asset.type == AssetType::SkeletalMesh)) return true;
    if (HasFilter(filters, ContentFilter::Blueprints) && asset.type == AssetType::Blueprint) return true;
    if (HasFilter(filters, ContentFilter::Scenes) &&
        (asset.type == AssetType::Scene || asset.type == AssetType::Prefab)) return true;
    if (HasFilter(filters, ContentFilter::Scripts) && asset.type == AssetType::Script) return true;
    if (HasFilter(filters, ContentFilter::Animations) && asset.type == AssetType::Animation) return true;
    if (HasFilter(filters, ContentFilter::Audio) && asset.type == AssetType::Audio) return true;
    if (HasFilter(filters, ContentFilter::Video) && asset.type == AssetType::Video) return true;
    if (HasFilter(filters, ContentFilter::Fonts) && asset.type == AssetType::Font) return true;
    return false;
}

bool FilterController::Matches(const AssetRecord& asset) const {
    if (m_ActiveFilters == ContentFilter::None) return true;

    bool matched = false;

    if (HasFilter(m_ActiveFilters, ContentFilter::Favorites)) {
        matched = matched || ContentAssetRegistry::Get().IsFavorite(asset.id);
    }
    if (HasFilter(m_ActiveFilters, ContentFilter::RecentlyModified)) {
        const auto now = std::chrono::system_clock::now().time_since_epoch().count();
        matched = matched || (asset.modifiedTime > static_cast<uint64_t>(now) - 10000000000ull);
    }
    if (HasFilter(m_ActiveFilters, ContentFilter::Referenced)) {
        matched = matched || asset.isReferenced;
    }
    if (HasFilter(m_ActiveFilters, ContentFilter::Unused)) {
        matched = matched || !asset.isReferenced;
    }

    const uint32_t typeMask =
        static_cast<uint32_t>(ContentFilter::Textures) |
        static_cast<uint32_t>(ContentFilter::Materials) |
        static_cast<uint32_t>(ContentFilter::Meshes) |
        static_cast<uint32_t>(ContentFilter::Blueprints) |
        static_cast<uint32_t>(ContentFilter::Scenes) |
        static_cast<uint32_t>(ContentFilter::Scripts) |
        static_cast<uint32_t>(ContentFilter::Animations) |
        static_cast<uint32_t>(ContentFilter::Audio) |
        static_cast<uint32_t>(ContentFilter::Video) |
        static_cast<uint32_t>(ContentFilter::Fonts) |
        static_cast<uint32_t>(ContentFilter::Folders);

    if ((static_cast<uint32_t>(m_ActiveFilters) & typeMask) != 0) {
        matched = matched || MatchesTypeFilter(asset, m_ActiveFilters);
    }

    return matched;
}

} // namespace we::editor::contentbrowser
