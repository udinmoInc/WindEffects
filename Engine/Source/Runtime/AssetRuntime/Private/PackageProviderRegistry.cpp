#include "AssetRuntime/PackageProviderRegistry.h"

#include <algorithm>

namespace we::runtime::assetruntime {

void PackageProviderRegistry::Register(std::shared_ptr<IPackageProvider> provider) {
    if (!provider) {
        return;
    }
    std::lock_guard lock(m_Mutex);
    m_ById[std::string(provider->GetProviderId())] = std::move(provider);
}

void PackageProviderRegistry::Unregister(std::string_view providerId) {
    std::lock_guard lock(m_Mutex);
    m_ById.erase(std::string(providerId));
}

void PackageProviderRegistry::Clear() {
    std::lock_guard lock(m_Mutex);
    m_ById.clear();
}

std::shared_ptr<IPackageProvider> PackageProviderRegistry::FindById(
    std::string_view providerId) const {
    std::lock_guard lock(m_Mutex);
    const auto it = m_ById.find(std::string(providerId));
    return it == m_ById.end() ? nullptr : it->second;
}

std::shared_ptr<IPackageProvider> PackageProviderRegistry::ResolveForPath(
    const std::filesystem::path& source) const {
    std::vector<std::shared_ptr<IPackageProvider>> candidates;
    {
        std::lock_guard lock(m_Mutex);
        for (const auto& [_, provider] : m_ById) {
            if (provider && provider->CanOpen(source)) {
                candidates.push_back(provider);
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

std::vector<std::shared_ptr<IPackageProvider>> PackageProviderRegistry::GetAll() const {
    std::lock_guard lock(m_Mutex);
    std::vector<std::shared_ptr<IPackageProvider>> out;
    out.reserve(m_ById.size());
    for (const auto& [_, provider] : m_ById) {
        out.push_back(provider);
    }
    return out;
}

} // namespace we::runtime::assetruntime
