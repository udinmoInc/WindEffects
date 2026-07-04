#pragma once

namespace we::programs::editor {

// Editor-only viewport rendering preferences.
class EditorPreferences {
public:
    static EditorPreferences& Get();

private:
    EditorPreferences() = default;
};

} // namespace we::programs::editor
