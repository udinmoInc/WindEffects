#include "ToolsPanelState.hpp"

#include "Core/EditorConfigPaths.hpp"

#include <filesystem>
#include <fstream>

namespace we::programs::editor {

namespace {

std::filesystem::path GetStatePath() {
    return we::core::ResolveEditorConfigPath("tools_panel.ini");
}

} // namespace

void ToolsPanelState::Load() {
    const auto path = GetStatePath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = line.substr(0, eq);
        const std::string value = line.substr(eq + 1);

        if (key.rfind("category.", 0) == 0) {
            categoryExpanded[key.substr(9)] = (value == "1");
        }
    }
}

void ToolsPanelState::Save() const {
    const auto path = GetStatePath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) return;

    for (const auto& [categoryId, expanded] : categoryExpanded) {
        file << "category." << categoryId << "=" << (expanded ? "1" : "0") << "\n";
    }
}

} // namespace we::programs::editor
