#include "AssetRuntime/AssetLoaderRegistry.h"

#include <algorithm>

namespace we::runtime::assetruntime {

void AssetLoaderRegistry::Register(std::shared_ptr<IAssetLoader> loader) {
    if (!loader) {
        return;
    }
    std::lock_guard lock(m_Mutex);
    m_ById[std::string(loader->GetLoaderId())] = std::move(loader);
}

void AssetLoaderRegistry::Unregister(std::string_view loaderId) {
    std::lock_guard lock(m_Mutex);
    m_ById.erase(std::string(loaderId));
}

void AssetLoaderRegistry::Clear() {
    std::lock_guard lock(m_Mutex);
    m_ById.clear();
}

std::shared_ptr<IAssetLoader> AssetLoaderRegistry::FindById(std::string_view loaderId) const {
    std::lock_guard lock(m_Mutex);
    const auto it = m_ById.find(std::string(loaderId));
    return it == m_ById.end() ? nullptr : it->second;
}

std::shared_ptr<IAssetLoader> AssetLoaderRegistry::Resolve(
    we::runtime::assetimporter::AssetKind kind,
    std::string_view contentType) const {
    std::vector<std::shared_ptr<IAssetLoader>> candidates;
    {
        std::lock_guard lock(m_Mutex);
        for (const auto& [_, loader] : m_ById) {
            if (loader && loader->CanLoad(kind, contentType)) {
                candidates.push_back(loader);
            }
        }
    }
    if (candidates.empty()) {
        return nullptr;
    }
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
        return a->GetPriority() > b->GetPriority();
    });
    return candidates.front();
}

std::vector<std::shared_ptr<IAssetLoader>> AssetLoaderRegistry::GetAll() const {
    std::lock_guard lock(m_Mutex);
    std::vector<std::shared_ptr<IAssetLoader>> out;
    out.reserve(m_ById.size());
    for (const auto& [_, loader] : m_ById) {
        out.push_back(loader);
    }
    return out;
}

} // namespace we::runtime::assetruntime
