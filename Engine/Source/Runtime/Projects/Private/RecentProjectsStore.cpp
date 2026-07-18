#include "Projects/RecentProjectsStore.h"

#include "JsonIO.h"
#include "Projects/ProjectLifecycle.h"

#include "Core/Logger.h"

#include <algorithm>
#include <cstdlib>

namespace we::projects {

RecentProjectsStore& RecentProjectsStore::Get() {
    static RecentProjectsStore instance;
    return instance;
}

std::filesystem::path RecentProjectsStore::StorePath() {
#if defined(_WIN32)
    const char* localApp = std::getenv("LOCALAPPDATA");
    std::filesystem::path base = localApp && localApp[0] != '\0'
        ? std::filesystem::path(localApp)
        : std::filesystem::path(".");
#else
    const char* home = std::getenv("HOME");
    std::filesystem::path base = home && home[0] != '\0'
        ? std::filesystem::path(home) / ".local" / "share"
        : std::filesystem::path(".");
#endif
    return base / "WindEffects" / "Editor" / "RecentProjects.json";
}

void RecentProjectsStore::Load() {
    m_Entries.clear();
    const auto path = StorePath();
    const auto json = JsonIO::TryLoad(path);
    if (!json) {
        return;
    }

    m_Limit = json->value("limit", 20);
    if (!json->contains("projects") || !(*json)["projects"].is_array()) {
        return;
    }

    for (const auto& item : (*json)["projects"]) {
        RecentProjectEntry entry{};
        entry.weprojPath = item.value("weprojPath", std::string{});
        entry.displayName = item.value("displayName", std::string{});
        entry.engineVersion = item.value("engineVersion", std::string{});
        entry.lastOpenedUtc = item.value("lastOpenedUtc", std::string{});
        if (entry.weprojPath.empty()) {
            continue;
        }
        if (!std::filesystem::exists(entry.weprojPath)) {
            continue;
        }
        m_Entries.push_back(std::move(entry));
    }
}

void RecentProjectsStore::Save() const {
    nlohmann::json root;
    root["limit"] = m_Limit;
    root["projects"] = nlohmann::json::array();
    for (const auto& entry : m_Entries) {
        root["projects"].push_back({
            { "weprojPath", entry.weprojPath },
            { "displayName", entry.displayName },
            { "engineVersion", entry.engineVersion },
            { "lastOpenedUtc", entry.lastOpenedUtc },
        });
    }
    if (!JsonIO::Save(StorePath(), root)) {
        HE_WARN("[RecentProjects] Failed to save recent projects list.");
    }
}

void RecentProjectsStore::Touch(
    const std::filesystem::path& weprojPath,
    const WeProjectDescriptor& descriptor) {
    const std::string pathUtf8 = weprojPath.string();
    m_Entries.erase(
        std::remove_if(
            m_Entries.begin(),
            m_Entries.end(),
            [&](const RecentProjectEntry& e) { return e.weprojPath == pathUtf8; }),
        m_Entries.end());

    RecentProjectEntry entry{};
    entry.weprojPath = pathUtf8;
    entry.displayName = descriptor.displayName.empty() ? descriptor.projectName : descriptor.displayName;
    entry.engineVersion = descriptor.engineVersion;
    entry.lastOpenedUtc = descriptor.lastOpenedUtc.empty()
        ? ProjectLifecycle::NowUtc()
        : descriptor.lastOpenedUtc;
    m_Entries.insert(m_Entries.begin(), std::move(entry));

    if (m_Limit > 0 && static_cast<int>(m_Entries.size()) > m_Limit) {
        m_Entries.resize(static_cast<size_t>(m_Limit));
    }
    Save();
}

void RecentProjectsStore::Remove(const std::filesystem::path& weprojPath) {
    const std::string pathUtf8 = weprojPath.string();
    const auto before = m_Entries.size();
    m_Entries.erase(
        std::remove_if(
            m_Entries.begin(),
            m_Entries.end(),
            [&](const RecentProjectEntry& e) { return e.weprojPath == pathUtf8; }),
        m_Entries.end());
    if (m_Entries.size() != before) {
        Save();
    }
}

void RecentProjectsStore::Clear() {
    m_Entries.clear();
    Save();
}

} // namespace we::projects
