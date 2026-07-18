#pragma once

#include "Projects/Export.h"
#include "Projects/WeProjectDescriptor.h"

#include <filesystem>
#include <string>
#include <vector>

namespace we::projects {

struct PROJECTS_API RecentProjectEntry {
    std::string weprojPath;
    std::string displayName;
    std::string engineVersion;
    std::string lastOpenedUtc;
};

class PROJECTS_API RecentProjectsStore {
public:
    static RecentProjectsStore& Get();

    void Load();
    void Save() const;

    [[nodiscard]] const std::vector<RecentProjectEntry>& Entries() const { return m_Entries; }

    void Touch(const std::filesystem::path& weprojPath, const WeProjectDescriptor& descriptor);
    void Remove(const std::filesystem::path& weprojPath);
    void Clear();

    void SetLimit(int limit) { m_Limit = limit < 0 ? 0 : limit; }
    [[nodiscard]] int Limit() const { return m_Limit; }

private:
    RecentProjectsStore() = default;

    [[nodiscard]] static std::filesystem::path StorePath();

    std::vector<RecentProjectEntry> m_Entries;
    int m_Limit = 20;
};

} // namespace we::projects
