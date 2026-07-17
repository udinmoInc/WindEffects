#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "KindUI/Core/Widget.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace we::editor::toolspanel {
using ::we::runtime::kindui::Widget;

struct EditorToolAction {
    std::string id;
    std::string categoryId;
    std::string label;
    std::string iconName;
    std::string shortcut;
    std::string keywords;
    int sortOrder = 0;
    bool favoritable = true;
    std::function<void()> onExecute;
    std::function<void()> onDragStart;
};

struct EditorToolCategory {
    std::string id;
    std::string modeId;
    std::string label;
    std::string iconName;
    int sortOrder = 0;
    bool defaultExpanded = true;
};

struct EditorToolMode {
    std::string id;
    std::string label;
    std::string iconName;
    std::string keywords;
    int sortOrder = 0;
    bool opensToolDrawerByDefault = true;
    using ContentFactory = std::function<std::shared_ptr<::we::runtime::kindui::Widget>(
        const EditorToolMode& mode,
        const std::string& searchFilter)>;
    ContentFactory customContent;
};

class UIFRAMEWORK_API EditorToolsRegistry {
public:
    static EditorToolsRegistry& Get();

    void RegisterMode(EditorToolMode mode);
    void RegisterCategory(EditorToolCategory category);
    void RegisterTool(EditorToolAction tool);

    [[nodiscard]] const EditorToolMode* FindMode(std::string_view modeId) const;
    [[nodiscard]] const EditorToolCategory* FindCategory(std::string_view categoryId) const;
    [[nodiscard]] const EditorToolAction* FindTool(std::string_view toolId) const;

    [[nodiscard]] std::vector<const EditorToolMode*> GetModesSorted() const;
    [[nodiscard]] std::vector<const EditorToolCategory*> GetCategoriesForMode(std::string_view modeId) const;
    [[nodiscard]] std::vector<const EditorToolAction*> GetToolsForCategory(std::string_view categoryId) const;

  /// Search tools/actions within a mode (empty modeId searches all modes).
    [[nodiscard]] std::vector<const EditorToolAction*> SearchTools(
        std::string_view query,
        std::string_view modeId = {}) const;

    void RecordToolUsage(std::string_view toolId);
    [[nodiscard]] const std::vector<std::string>& GetRecentToolIds() const { return m_RecentToolIds; }

    void ToggleFavorite(std::string_view toolId);
    [[nodiscard]] bool IsFavorite(std::string_view toolId) const;
    [[nodiscard]] std::vector<const EditorToolAction*> GetFavoriteTools(std::string_view modeId) const;

    void SetFavorite(std::string_view toolId, bool enabled);
    void LoadFavorites(const std::unordered_map<std::string, bool>& favorites);
    [[nodiscard]] const std::unordered_map<std::string, bool>& GetFavoriteStates() const { return m_Favorites; }

private:
    EditorToolsRegistry() = default;

#pragma warning(push)
#pragma warning(disable: 4251)
    std::unordered_map<std::string, EditorToolMode> m_Modes;
    std::unordered_map<std::string, EditorToolCategory> m_Categories;
    std::unordered_map<std::string, EditorToolAction> m_Tools;
    std::vector<std::string> m_RecentToolIds;
    std::unordered_map<std::string, bool> m_Favorites;
#pragma warning(pop)
};

#define REGISTER_EDITOR_TOOL_MODE_IMPL(ModeId, Label, IconName, SortOrder, OpensDrawer) \
    namespace { \
        struct EditorToolModeRegister_##ModeId { \
            EditorToolModeRegister_##ModeId() { \
                ::we::editor::toolspanel::EditorToolMode mode; \
                mode.id = #ModeId; \
                mode.label = Label; \
                mode.iconName = IconName; \
                mode.sortOrder = SortOrder; \
                mode.keywords = Label; \
                mode.opensToolDrawerByDefault = OpensDrawer; \
                ::we::editor::toolspanel::EditorToolsRegistry::Get().RegisterMode(std::move(mode)); \
            } \
        }; \
        static EditorToolModeRegister_##ModeId g_EditorToolModeRegister_##ModeId; \
    }

#define REGISTER_EDITOR_TOOL_MODE(ModeId, Label, IconName, SortOrder) \
    REGISTER_EDITOR_TOOL_MODE_IMPL(ModeId, Label, IconName, SortOrder, true)

#define REGISTER_EDITOR_TOOL_MODE_COMPACT(ModeId, Label, IconName, SortOrder) \
    REGISTER_EDITOR_TOOL_MODE_IMPL(ModeId, Label, IconName, SortOrder, false)

#define REGISTER_EDITOR_TOOL_CATEGORY(ModeId, CategoryId, Label, IconName, SortOrder) \
    namespace { \
        struct EditorToolCategoryRegister_##CategoryId { \
            EditorToolCategoryRegister_##CategoryId() { \
                ::we::editor::toolspanel::EditorToolCategory category; \
                category.id = #CategoryId; \
                category.modeId = #ModeId; \
                category.label = Label; \
                category.iconName = IconName; \
                category.sortOrder = SortOrder; \
                ::we::editor::toolspanel::EditorToolsRegistry::Get().RegisterCategory(std::move(category)); \
            } \
        }; \
        static EditorToolCategoryRegister_##CategoryId g_EditorToolCategoryRegister_##CategoryId; \
    }

#define REGISTER_EDITOR_TOOL(CategoryId, ToolId, Label, IconName, Shortcut, ExecuteFunc) \
    namespace { \
        struct EditorToolRegister_##ToolId { \
            EditorToolRegister_##ToolId() { \
                ::we::editor::toolspanel::EditorToolAction tool; \
                tool.id = #ToolId; \
                tool.categoryId = #CategoryId; \
                tool.label = Label; \
                tool.iconName = IconName; \
                tool.shortcut = Shortcut; \
                tool.keywords = Label; \
                tool.onExecute = ExecuteFunc; \
                ::we::editor::toolspanel::EditorToolsRegistry::Get().RegisterTool(std::move(tool)); \
            } \
        }; \
        static EditorToolRegister_##ToolId g_EditorToolRegister_##ToolId; \
    }

} // namespace we::editor::toolspanel
