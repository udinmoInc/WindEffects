#include "AssetImporter/ImporterRegistry.h"

#include "AssetImporter/Types.h"

#include <algorithm>

namespace we::runtime::assetimporter {

void ImporterRegistry::Register(std::shared_ptr<IAssetImporter> importer) {
    if (!importer) {
        return;
    }
    std::lock_guard lock(m_Mutex);
    m_ById[std::string(importer->GetImporterId())] = std::move(importer);
}

void ImporterRegistry::Unregister(std::string_view importerId) {
    std::lock_guard lock(m_Mutex);
    m_ById.erase(std::string(importerId));
}

void ImporterRegistry::Clear() {
    std::lock_guard lock(m_Mutex);
    m_ById.clear();
}

std::shared_ptr<IAssetImporter> ImporterRegistry::FindById(std::string_view importerId) const {
    std::lock_guard lock(m_Mutex);
    const auto it = m_ById.find(std::string(importerId));
    return it == m_ById.end() ? nullptr : it->second;
}

std::shared_ptr<IAssetImporter> ImporterRegistry::ResolveForPath(
    const std::filesystem::path& sourcePath) const {
    std::vector<std::shared_ptr<IAssetImporter>> candidates;
    {
        std::lock_guard lock(m_Mutex);
        for (const auto& [_, importer] : m_ById) {
            if (importer && importer->CanImport(sourcePath)) {
                candidates.push_back(importer);
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

std::shared_ptr<IAssetImporter> ImporterRegistry::ResolveForKind(AssetKind kind) const {
    std::vector<std::shared_ptr<IAssetImporter>> candidates;
    {
        std::lock_guard lock(m_Mutex);
        for (const auto& [_, importer] : m_ById) {
            if (importer && importer->GetAssetKind() == kind) {
                candidates.push_back(importer);
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

std::vector<std::shared_ptr<IAssetImporter>> ImporterRegistry::GetAll() const {
    std::lock_guard lock(m_Mutex);
    std::vector<std::shared_ptr<IAssetImporter>> out;
    out.reserve(m_ById.size());
    for (const auto& [_, importer] : m_ById) {
        out.push_back(importer);
    }
    return out;
}

} // namespace we::runtime::assetimporter
