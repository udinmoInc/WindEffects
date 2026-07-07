#pragma once

#include <string>
#include <unordered_map>

namespace we::programs::editor {

struct ToolsPanelState {
    std::unordered_map<std::string, bool> categoryExpanded;

    void Load();
    void Save() const;
};

} // namespace we::programs::editor
