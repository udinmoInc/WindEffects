#pragma once

#include "Core/Geometry.hpp"
#include <string>
#include <unordered_map>

namespace we::UI {

class PaintContext;

class IconRegistry {
public:
    static IconRegistry& Get();

    void RegisterIcon(const std::string& name, const std::string& svgPath) {
        m_Icons[name] = svgPath;
    }

    std::string GetIconPath(const std::string& name) const {
        auto it = m_Icons.find(name);
        return it != m_Icons.end() ? it->second : "";
    }

    bool HasIcon(const std::string& name) const {
        return m_Icons.find(name) != m_Icons.end();
    }

    void InitializeDefaultIcons();

private:
    IconRegistry() = default;
    std::unordered_map<std::string, std::string> m_Icons;
};

class IconPainter {
public:
    static void DrawIcon(PaintContext& context, const std::string& iconName,
        const Point& position, float size, const Color& color);

    static void DrawIcon(PaintContext& context, const std::string& iconName,
        const Rect& bounds, const Color& color);

    static void DrawVerticalMoreMenu(PaintContext& context, const Rect& bounds, const Color& color);
};

// Lucide icon names (kebab-case, matching Engine/Content/Icons/icons/*.svg).
namespace Icons {

    constexpr const char* CursorName     = "mouse-pointer-2";
    constexpr const char* MoveName       = "move";
    constexpr const char* RotateName     = "rotate-cw";
    constexpr const char* ScaleName      = "scaling";
    constexpr const char* PlayName       = "play";
    constexpr const char* PauseName      = "pause";
    constexpr const char* StopName       = "square";
    constexpr const char* PerspectiveName = "box";
    constexpr const char* LitName        = "sun";
    constexpr const char* WireframeName  = "grid-3x3";
    constexpr const char* CameraName     = "camera";
    constexpr const char* SnapName       = "magnet";
    constexpr const char* GridName       = "grid-3x3";
    constexpr const char* CubeName       = "box";
    constexpr const char* SphereName     = "circle";
    constexpr const char* PlaneName      = "square";
    constexpr const char* CylinderName   = "cylinder";
    constexpr const char* LightName      = "lightbulb";
    constexpr const char* SunName        = "sun";
    constexpr const char* PointLightName = "lightbulb";
    constexpr const char* MaterialName   = "palette";
    constexpr const char* ShaderName     = "code-2";
    constexpr const char* TextureName    = "image";
    constexpr const char* SaveName       = "save";
    constexpr const char* SaveAllName    = "save-all";
    constexpr const char* OpenName       = "folder-open";
    constexpr const char* NewName        = "file-plus";
    constexpr const char* FolderName     = "folder";
    constexpr const char* DocumentName   = "file";
    constexpr const char* CodeName       = "code";
    constexpr const char* BuildName      = "wrench";
    constexpr const char* PackageName    = "package";
    constexpr const char* UndoName       = "undo";
    constexpr const char* RedoName       = "redo";
    constexpr const char* CopyName       = "copy";
    constexpr const char* PasteName      = "clipboard";
    constexpr const char* DeleteName     = "trash-2";
    constexpr const char* SearchName     = "search";
    constexpr const char* SettingsName   = "settings";
    constexpr const char* MenuName       = "menu";
    constexpr const char* MoreName       = "more-vertical";
    constexpr const char* ChevronRightName = "chevron-right";
    constexpr const char* ChevronDownName  = "chevron-down";
    constexpr const char* ChevronLeftName  = "chevron-left";
    constexpr const char* ArrowLeftName    = "arrow-left";
    constexpr const char* ArrowRightName   = "arrow-right";
    constexpr const char* EyeName        = "eye";
    constexpr const char* EyeOffName     = "eye-off";
    constexpr const char* LockName       = "lock";
    constexpr const char* UnlockName     = "unlock";
    constexpr const char* LayersName     = "layers";
    constexpr const char* HierarchyName  = "list-tree";
    constexpr const char* PropertiesName = "sliders-horizontal";
    constexpr const char* ConsoleName    = "terminal";
    constexpr const char* ProfilerName   = "activity";
    constexpr const char* ListName       = "list";
    constexpr const char* RefreshName    = "refresh-cw";
    constexpr const char* StarName       = "star";
    constexpr const char* StarFilledName = "star";
    constexpr const char* FilterName     = "filter";
    constexpr const char* PlusName       = "plus";
    constexpr const char* MinusName      = "minus";
    constexpr const char* XName          = "x";
    constexpr const char* CheckName      = "check";
    constexpr const char* InfoName       = "info";
    constexpr const char* WarningName    = "alert-triangle";
    constexpr const char* ErrorName      = "x-circle";
    constexpr const char* SuccessName    = "check";
    constexpr const char* MinimizeName   = "minus";
    constexpr const char* MaximizeName   = "maximize-2";
    constexpr const char* RestoreName    = "maximize";
    constexpr const char* CompassName    = "compass";
    constexpr const char* EraserName     = "eraser";
    constexpr const char* BrushName      = "paintbrush";
    constexpr const char* RecordName     = "circle";

    inline std::string ResolveLucideName(const std::string& name) {
        static const std::unordered_map<std::string, std::string> kAliases = {
            {"cursor", CursorName},
            {"move", MoveName},
            {"rotate", RotateName},
            {"scale", ScaleName},
            {"play", PlayName},
            {"pause", PauseName},
            {"stop", StopName},
            {"perspective", PerspectiveName},
            {"lit", LitName},
            {"wireframe", WireframeName},
            {"camera", CameraName},
            {"snap", SnapName},
            {"grid", GridName},
            {"cube", CubeName},
            {"sphere", SphereName},
            {"plane", PlaneName},
            {"cylinder", CylinderName},
            {"light", LightName},
            {"sun", SunName},
            {"point-light", PointLightName},
            {"material", MaterialName},
            {"material_instance", "layers"},
            {"shader", ShaderName},
            {"texture", TextureName},
            {"save", SaveName},
            {"open", OpenName},
            {"new", NewName},
            {"folder", FolderName},
            {"document", DocumentName},
            {"code", CodeName},
            {"build", BuildName},
            {"package", PackageName},
            {"undo", UndoName},
            {"redo", RedoName},
            {"copy", CopyName},
            {"paste", PasteName},
            {"delete", DeleteName},
            {"search", SearchName},
            {"settings", SettingsName},
            {"menu", MenuName},
            {"more", MoreName},
            {"ellipsis-vertical", MoreName},
            {"chevron-right", ChevronRightName},
            {"chevron-down", ChevronDownName},
            {"chevron-left", ChevronLeftName},
            {"eye", EyeName},
            {"eye-off", EyeOffName},
            {"lock", LockName},
            {"unlock", UnlockName},
            {"layers", LayersName},
            {"hierarchy", HierarchyName},
            {"properties", PropertiesName},
            {"console", ConsoleName},
            {"profiler", ProfilerName},
            {"list", ListName},
            {"refresh", RefreshName},
            {"star", StarName},
            {"star-filled", StarFilledName},
            {"plus", PlusName},
            {"minus", MinusName},
            {"x", XName},
            {"check", CheckName},
            {"info", InfoName},
            {"warning", WarningName},
            {"error", ErrorName},
            {"success", SuccessName},
            {"minimize", MinimizeName},
            {"maximize", MaximizeName},
            {"restore", RestoreName},
            {"compass", CompassName},
            {"eraser", EraserName},
            {"brush", BrushName},
            {"record", RecordName},
            {"blueprint", "blocks"},
            {"static_mesh", "box"},
            {"skeletal_mesh", "bone"},
            {"audio", "volume-2"},
            {"font", "type"},
            {"script", "file-code"},
            {"scene", "map"},
            {"level", "map"},
            {"animation", "clapperboard"},
            {"particle", "sparkles"},
            {"collection", "library"},
            {"plugin", "plug"},
            {"engine", "cpu"},
            {"project", "folder-kanban"},
            {"favorites", "star"},
        };

        auto it = kAliases.find(name);
        if (it != kAliases.end()) return it->second;
        return name;
    }

    inline bool IsKnownIcon(const std::string& name) {
        const std::string resolved = ResolveLucideName(name);
        return !resolved.empty() && resolved != "circle-help";
    }
}

} // namespace we::UI
