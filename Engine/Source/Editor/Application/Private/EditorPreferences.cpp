#include "EditorPreferences.hpp"

namespace we::programs::editor {

EditorPreferences& EditorPreferences::Get() {
    static EditorPreferences instance;
    return instance;
}

} // namespace we::programs::editor
