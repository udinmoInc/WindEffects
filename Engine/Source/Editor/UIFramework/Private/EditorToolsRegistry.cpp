#include "EditorToolsRegistry.h"

#include <algorithm>
#include <cctype>

namespace we::programs::editor {

namespace {

std::string ToLower(std::string value) {
    for (char& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

bool ContainsInsensitive(std::string_view haystack, std::string_view needle) {
    if (needle.empty()) return true;
    const std::string h = ToLower(std::string(haystack));
    const std::string n = ToLower(std::string(needle));
    return h.find(n) != std::string::npos;
}

} // namespace

EditorToolsRegistry& EditorToolsRegistry::Get() {
    static EditorToolsRegistry instance;
    return instance;
}

void EditorToolsRegistry::RegisterMode(EditorToolMode mode) {
    m_Modes[mode.id] = std::move(mode);
}

void EditorToolsRegistry::RegisterCategory(EditorToolCategory category) {
    m_Categories[category.id] = std::move(category);
}

void EditorToolsRegistry::RegisterTool(EditorToolAction tool) {
    m_Tools[tool.id] = std::move(tool);
}

const EditorToolMode* EditorToolsRegistry::FindMode(std::string_view modeId) const {
    auto it = m_Modes.find(std::string(modeId));
    return it != m_Modes.end() ? &it->second : nullptr;
}

const EditorToolCategory* EditorToolsRegistry::FindCategory(std::string_view categoryId) const {
    auto it = m_Categories.find(std::string(categoryId));
    return it != m_Categories.end() ? &it->second : nullptr;
}

const EditorToolAction* EditorToolsRegistry::FindTool(std::string_view toolId) const {
    auto it = m_Tools.find(std::string(toolId));
    return it != m_Tools.end() ? &it->second : nullptr;
}

std::vector<const EditorToolMode*> EditorToolsRegistry::GetModesSorted() const {
    std::vector<const EditorToolMode*> modes;
    modes.reserve(m_Modes.size());
    for (const auto& [_, mode] : m_Modes) {
        modes.push_back(&mode);
    }
    std::sort(modes.begin(), modes.end(), [](const EditorToolMode* a, const EditorToolMode* b) {
        if (a->sortOrder != b->sortOrder) return a->sortOrder < b->sortOrder;
        return a->label < b->label;
    });
    return modes;
}

std::vector<const EditorToolCategory*> EditorToolsRegistry::GetCategoriesForMode(std::string_view modeId) const {
    std::vector<const EditorToolCategory*> categories;
    for (const auto& [_, category] : m_Categories) {
        if (category.modeId == modeId) {
            categories.push_back(&category);
        }
    }
    std::sort(categories.begin(), categories.end(), [](const EditorToolCategory* a, const EditorToolCategory* b) {
        if (a->sortOrder != b->sortOrder) return a->sortOrder < b->sortOrder;
        return a->label < b->label;
    });
    return categories;
}

std::vector<const EditorToolAction*> EditorToolsRegistry::GetToolsForCategory(std::string_view categoryId) const {
    std::vector<const EditorToolAction*> tools;
    for (const auto& [_, tool] : m_Tools) {
        if (tool.categoryId == categoryId) {
            tools.push_back(&tool);
        }
    }
    std::sort(tools.begin(), tools.end(), [](const EditorToolAction* a, const EditorToolAction* b) {
        if (a->sortOrder != b->sortOrder) return a->sortOrder < b->sortOrder;
        return a->label < b->label;
    });
    return tools;
}

std::vector<const EditorToolAction*> EditorToolsRegistry::SearchTools(
    std::string_view query,
    std::string_view modeId) const
{
    std::vector<const EditorToolAction*> results;
    for (const auto& [_, tool] : m_Tools) {
        const EditorToolCategory* category = FindCategory(tool.categoryId);
        if (!category) continue;
        if (!modeId.empty() && category->modeId != modeId) continue;

        if (ContainsInsensitive(tool.label, query) ||
            ContainsInsensitive(tool.keywords, query) ||
            ContainsInsensitive(tool.shortcut, query) ||
            ContainsInsensitive(category->label, query))
        {
            results.push_back(&tool);
        }
    }
    std::sort(results.begin(), results.end(), [](const EditorToolAction* a, const EditorToolAction* b) {
        if (a->sortOrder != b->sortOrder) return a->sortOrder < b->sortOrder;
        return a->label < b->label;
    });
    return results;
}

void EditorToolsRegistry::RecordToolUsage(std::string_view toolId) {
    const std::string id(toolId);
    auto it = std::find(m_RecentToolIds.begin(), m_RecentToolIds.end(), id);
    if (it != m_RecentToolIds.end()) {
        m_RecentToolIds.erase(it);
    }
    m_RecentToolIds.insert(m_RecentToolIds.begin(), id);
    if (m_RecentToolIds.size() > 12) {
        m_RecentToolIds.resize(12);
    }
}

void EditorToolsRegistry::ToggleFavorite(std::string_view toolId) {
    const std::string id(toolId);
    m_Favorites[id] = !IsFavorite(toolId);
}

bool EditorToolsRegistry::IsFavorite(std::string_view toolId) const {
    auto it = m_Favorites.find(std::string(toolId));
    return it != m_Favorites.end() && it->second;
}

std::vector<const EditorToolAction*> EditorToolsRegistry::GetFavoriteTools(std::string_view modeId) const {
    std::vector<const EditorToolAction*> favorites;
    for (const auto& [toolId, enabled] : m_Favorites) {
        if (!enabled) continue;
        const EditorToolAction* tool = FindTool(toolId);
        if (!tool) continue;
        const EditorToolCategory* category = FindCategory(tool->categoryId);
        if (!category) continue;
        if (!modeId.empty() && category->modeId != modeId) continue;
        favorites.push_back(tool);
    }
    return favorites;
}

} // namespace we::programs::editor
