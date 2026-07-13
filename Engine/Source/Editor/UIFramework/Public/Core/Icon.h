#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include "Core/Geometry.h"
#include <string>
#include <unordered_map>

namespace WindEffects::Editor::UI {

class PaintContext;

#pragma warning(push)
#pragma warning(disable: 4251)

class UIFRAMEWORK_API IconRegistry {
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

    // Registered editor SVGs carry their own fill colors and need full-color compositing.
    bool IsEditorSvgIcon(const std::string& name) const { return HasIcon(name); }

    void InitializeDefaultIcons();

private:
    IconRegistry() = default;
    std::unordered_map<std::string, std::string> m_Icons;
};

#pragma warning(pop)

class UIFRAMEWORK_API IconPainter {
public:
    static void DrawIcon(PaintContext& context, const std::string& iconName,
        const Point& position, float size, const Color& color);

    static void DrawIcon(PaintContext& context, const std::string& iconName,
        const Rect& bounds, const Color& color);

    static void DrawVerticalMoreMenu(PaintContext& context, const Rect& bounds, const Color& color);
};

// Icon names resolve to atlas entries in icons.weiconmeta (PascalCase stripped to lowercase).
namespace Icons {

    constexpr const char* CursorName     = "cursor";
    constexpr const char* MoveName       = "move";
    constexpr const char* RotateName     = "rotate";
    constexpr const char* ScaleName      = "scale";
    constexpr const char* PlayName       = "play";
    constexpr const char* PlaySolidName  = "playsolid";
    constexpr const char* PauseName      = "pause";
    constexpr const char* StopName       = "stop";
    constexpr const char* PerspectiveName = "box";
    constexpr const char* LitName        = "sun";
    constexpr const char* WireframeName  = "grid";
    constexpr const char* CameraName     = "camera";
    constexpr const char* SnapName       = "magnet";
    constexpr const char* GridName       = "grid";
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
    constexpr const char* SaveAllName    = "save";
    constexpr const char* OpenName       = "folder-open";
    constexpr const char* NewName        = "file-plus";
    constexpr const char* FolderName     = "folder";
    constexpr const char* ProjectFolderName = "folder-kanban";
    constexpr const char* ToolbarObjectName = "object";
    constexpr const char* ToolbarEnvironmentName = "sun";
    constexpr const char* GlobeName      = "sun";
    constexpr const char* UserName       = "user";
    constexpr const char* MountainName   = "mountain";
    constexpr const char* TreesName      = "trees";
    constexpr const char* VideoName      = "video";
    constexpr const char* ConeName       = "cone";
    constexpr const char* CapsuleName    = "pill";
    constexpr const char* FlashlightName = "flashlight";
    constexpr const char* BlocksName     = "blocks";
    constexpr const char* BrainName      = "brain";
    constexpr const char* LayoutPanelName = "layout";
    constexpr const char* StickyNoteName = "sticky-note";
    constexpr const char* CrosshairName  = "crosshair";
    constexpr const char* ComponentName  = "component";
    constexpr const char* SparklesName   = "sparkles";
    constexpr const char* Volume2Name    = "volume-2";
    constexpr const char* ZapName        = "zap";
    constexpr const char* MapName        = "map";
    constexpr const char* DocumentName   = "file";
    constexpr const char* CodeName       = "code";
    constexpr const char* BuildName      = "wrench";
    constexpr const char* PackageName    = "package";
    constexpr const char* MonitorName    = "monitor";
    constexpr const char* UndoName       = "undo";
    constexpr const char* RedoName       = "redo";
    constexpr const char* CopyName       = "copy";
    constexpr const char* PasteName      = "clipboard";
    constexpr const char* DeleteName     = "trash-2";
    constexpr const char* SearchName     = "search";
    constexpr const char* SettingsName   = "settings";
    constexpr const char* MenuName       = "menu";
    constexpr const char* MoreName       = "more-vertical";
    constexpr const char* ChevronRightName = "chevronright";
    constexpr const char* ChevronDownName  = "chevrondown";
    constexpr const char* ChevronLeftName  = "chevronleft";
    constexpr const char* ChevronUpName    = "chevronup";
    constexpr const char* ArrowLeftName    = "arrow-left";
    constexpr const char* ArrowRightName   = "arrow-right";
    constexpr const char* EyeName        = "eye";
    constexpr const char* EyeOffName     = "eyeoff";
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
    constexpr const char* XName          = "close";
    constexpr const char* CheckName      = "check";
    constexpr const char* InfoName       = "info";
    constexpr const char* WarningName    = "alert-triangle";
    constexpr const char* ErrorName      = "x-circle";
    constexpr const char* SuccessName    = "check";
    constexpr const char* MinimizeName   = "minus";
    constexpr const char* MaximizeName   = "square";
    constexpr const char* RestoreName    = "copy";
    constexpr const char* CompassName    = "compass";
    constexpr const char* EraserName     = "eraser";
    constexpr const char* BrushName      = "paintbrush";
    constexpr const char* RecordName     = "circle";

    inline std::string ResolveLucideName(const std::string& name) {
        static const std::unordered_map<std::string, std::string> kAliases = {
            {"cursor", CursorName},
            {"mouse-pointer-2", CursorName},
            {"move", MoveName},
            {"rotate", RotateName},
            {"rotate-cw", RotateName},
            {"scale", ScaleName},
            {"scaling", ScaleName},
            {"play", PlayName},
            {"playsolid", PlaySolidName},
            {"pause", PauseName},
            {"stop", StopName},
            {"square", StopName},
            {"perspective", PerspectiveName},
            {"lit", LitName},
            {"wireframe", WireframeName},
            {"grid-3x3", GridName},
            {"grid", GridName},
            {"camera", CameraName},
            {"snap", SnapName},
            {"magnet", SnapName},
            {"cube", CubeName},
            {"sphere", SphereName},
            {"plane", PlaneName},
            {"cylinder", CylinderName},
            {"light", LightName},
            {"sun", SunName},
            {"globe", SunName},
            {"toolbar-environment", ToolbarEnvironmentName},
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
            {"chevronright", ChevronRightName},
            {"chevron-down", ChevronDownName},
            {"chevrondown", ChevronDownName},
            {"chevron-left", ChevronLeftName},
            {"chevronleft", ChevronLeftName},
            {"chevron-up", ChevronUpName},
            {"chevronup", ChevronUpName},
            {"eye", EyeName},
            {"eye-off", EyeOffName},
            {"eyeoff", EyeOffName},
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
            {"close", XName},
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
            {"component", ToolbarObjectName},
            {"toolbar-object", ToolbarObjectName},
            {"object", ToolbarObjectName},
            {"filter", FilterName},
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

} // namespace WindEffects::Editor::UI
