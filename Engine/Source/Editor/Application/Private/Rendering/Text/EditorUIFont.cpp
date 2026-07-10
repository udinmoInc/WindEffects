#include "Rendering/Text/EditorUIFont.h"

#include "Core/Logger.h"

#include <filesystem>

namespace we::UI::Text {

namespace {

bool FileExists(const std::string& path) {
    return !path.empty() && std::filesystem::exists(path);
}

std::string FirstExisting(const std::initializer_list<const char*>& paths) {
    for (const char* path : paths) {
        if (FileExists(path)) {
            return path;
        }
    }
    return {};
}

} // namespace

EditorUIFontPaths ResolveEditorUIFonts() {
    EditorUIFontPaths fonts{};

    fonts.fallback = FirstExisting({
        "Assets/Fonts/Inter-Regular.ttf",
        "Engine/Content/Fonts/Inter-Regular.ttf",
        "../Assets/Fonts/Inter-Regular.ttf",
    });

#if defined(_WIN32)
    fonts.regular = FirstExisting({
        "C:/Windows/Fonts/SegoeUIVF.ttf",
        "C:/Windows/Fonts/segoeuivf.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/SegoeUI.ttf",
    });
    fonts.semibold = FirstExisting({
        "C:/Windows/Fonts/seguisb.ttf",
        "C:/Windows/Fonts/SegoeUI-Semibold.ttf",
    });
    if (!fonts.regular.empty()) {
        fonts.familyName = "Segoe UI";
    }
#endif

    if (fonts.regular.empty()) {
        fonts.regular = fonts.fallback;
    }
    if (fonts.semibold.empty()) {
        fonts.semibold = FirstExisting({
            "Assets/Fonts/Inter-Bold.ttf",
            "Engine/Content/Fonts/Inter-Bold.ttf",
            "../Assets/Fonts/Inter-Bold.ttf",
            fonts.fallback.c_str(),
        });
    }
    if (fonts.familyName.empty()) {
        fonts.familyName = "Inter";
    }

    return fonts;
}

} // namespace we::UI::Text
