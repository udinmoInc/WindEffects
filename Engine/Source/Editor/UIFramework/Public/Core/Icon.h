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

    // 12px layout using the 16px atlas tier (dock-tab close, dropdown chevrons).
    static void DrawCompactIcon(PaintContext& context, const std::string& iconName,
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
    constexpr const char* PerspectiveName = "3dcube";
    constexpr const char* LitName        = "lit";
    constexpr const char* WireframeName  = "wireframe";
    constexpr const char* CameraName     = "pivot";
    constexpr const char* SnapName       = "magnet";
    constexpr const char* GridName       = "grid";
    constexpr const char* CubeName       = "3dcube";
    constexpr const char* SphereName     = "3dsphere";
    constexpr const char* PlaneName      = "square";
    constexpr const char* CylinderName   = "3dcylinder";
    // Full-color 3D primitive thumbnails baked into the UI icon atlas (tiers 32+).
    constexpr const char* Cube3DName        = "3dcube";
    constexpr const char* Sphere3DName      = "3dsphere";
    constexpr const char* Cylinder3DName    = "3dcylinder";
    constexpr const char* Plane3DName       = "3dplane";
    constexpr const char* Cone3DName        = "3dcone";
    constexpr const char* Capsule3DName     = "3dcapsule";
    constexpr const char* BlankActor3DName  = "3dblankactor";
    constexpr const char* LightName      = "sun";
    constexpr const char* SunName        = "sun";
    constexpr const char* PointLightName = "sun";
    constexpr const char* MaterialName   = "lit";
    constexpr const char* ShaderName     = "terminal";
    constexpr const char* TextureName    = "contentbrowser";
    constexpr const char* SaveName       = "save";
    constexpr const char* SaveAllName    = "saveall";
    constexpr const char* AddActorName   = "addactor";
    constexpr const char* PivotName      = "pivot";
    constexpr const char* OpenFolderName = "openfolder";
    constexpr const char* NewFileName    = "newfile";
    constexpr const char* OpenName       = OpenFolderName;
    constexpr const char* NewName        = NewFileName;
    constexpr const char* FolderName     = "openfolder";
    constexpr const char* ContentBrowserName = "contentbrowser";
    constexpr const char* RecentName     = "recent";
    constexpr const char* ProjectFolderName = "openfolder";
    constexpr const char* ToolbarObjectName = "object";
    constexpr const char* ToolbarEnvironmentName = "sun";
    constexpr const char* GlobeName      = "globe";
    constexpr const char* UserName       = "object";
    constexpr const char* MountainName   = "globe";
    constexpr const char* TreesName      = "globe";
    constexpr const char* VideoName      = "mediaplay";
    constexpr const char* ConeName       = "3dcone";
    constexpr const char* CapsuleName    = "3dcapsule";
    constexpr const char* FlashlightName = "sun";
    constexpr const char* BlocksName     = "component";
    constexpr const char* BrainName      = "object";
    constexpr const char* LayoutPanelName = "grid";
    constexpr const char* StickyNoteName = "newfile";
    constexpr const char* CrosshairName  = "pivot";
    constexpr const char* ComponentName  = "component";
    constexpr const char* SparklesName   = "lit";
    constexpr const char* Volume2Name    = "mediaplay";
    constexpr const char* ZapName        = "lit";
    constexpr const char* MapName        = "globe";
    constexpr const char* DocumentName   = "newfile";
    constexpr const char* CodeName       = "terminal";
    constexpr const char* BuildName      = "wrench";
    constexpr const char* PackageName    = "object";
    constexpr const char* MonitorName    = "sun";
    constexpr const char* UndoName       = "undo";
    constexpr const char* RedoName       = "redo";
    constexpr const char* CopyName       = "copy";
    constexpr const char* PasteName      = "copy";
    constexpr const char* DeleteName     = "close";
    constexpr const char* SearchName     = "search";
    constexpr const char* SettingsName   = "settings";
    constexpr const char* MenuName       = "settings";
    constexpr const char* MoreName       = "chevronright";
    constexpr const char* ChevronRightName = "chevronright";
    constexpr const char* ChevronDownName  = "chevrondown";
    constexpr const char* ChevronLeftName  = "chevronleft";
    constexpr const char* ChevronUpName    = "chevronup";
    constexpr const char* ArrowLeftName    = "chevronleft";
    constexpr const char* ArrowRightName   = "chevronright";
    constexpr const char* EyeName        = "eye";
    constexpr const char* EyeOffName     = "eyeoff";
    constexpr const char* LockName       = "settings";
    constexpr const char* UnlockName     = "settings";
    constexpr const char* LayersName     = "contentbrowser";
    constexpr const char* HierarchyName  = "object";
    constexpr const char* PropertiesName = "settings";
    constexpr const char* TerminalName   = "terminal";
    constexpr const char* OutputLogName  = "outputlog";
    constexpr const char* ConsoleName    = TerminalName;
    constexpr const char* MediaPlayName  = "mediaplay";
    constexpr const char* ProfilerName   = "wrench";
    constexpr const char* ListName       = "object";
    constexpr const char* RefreshName    = "redo";
    constexpr const char* StarName       = "sun";
    constexpr const char* StarFilledName = "lit";
    constexpr const char* FilterName     = "filter";
    constexpr const char* PlusName       = "plus";
    constexpr const char* MinusName      = "minus";
    constexpr const char* XName          = "close";
    constexpr const char* CheckName      = "save";
    constexpr const char* InfoName       = "info";
    constexpr const char* WarningName    = "info";
    constexpr const char* ErrorName      = "close";
    constexpr const char* SuccessName    = "save";
    constexpr const char* MinimizeName   = "minus";
    constexpr const char* MaximizeName   = "maximize";
    constexpr const char* RestoreName    = "copy";
    constexpr const char* CompassName    = "globe";
    constexpr const char* EraserName     = "close";
    constexpr const char* BrushName      = "lit";
    constexpr const char* RecordName     = "stop";

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
            {"3dcube", Cube3DName},
            {"3dsphere", Sphere3DName},
            {"3dcylinder", Cylinder3DName},
            {"3dplane", Plane3DName},
            {"3dcone", Cone3DName},
            {"3dcapsule", Capsule3DName},
            {"3dblankactor", BlankActor3DName},
            {"3d-blank-actor", BlankActor3DName},
            {"light", LightName},
            {"sun", SunName},
            {"globe", GlobeName},
            {"toolbar-environment", ToolbarEnvironmentName},
            {"point-light", PointLightName},
            {"material", MaterialName},
            {"material_instance", LayersName},
            {"shader", ShaderName},
            {"texture", TextureName},
            {"save", SaveName},
            {"save-all", SaveAllName},
            {"saveall", SaveAllName},
            {"addactor", AddActorName},
            {"add-actor", AddActorName},
            {"pivot", PivotName},
            {"openfolder", OpenFolderName},
            {"open-folder", OpenFolderName},
            {"folder-open", OpenFolderName},
            {"newfile", NewFileName},
            {"new-file", NewFileName},
            {"file-plus", NewFileName},
            {"open", OpenName},
            {"new", NewName},
            {"folder", FolderName},
            {"content-browser", ContentBrowserName},
            {"contentbrowser", ContentBrowserName},
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
            {"more-vertical", MoreName},
            {"list-tree", HierarchyName},
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
            {"terminal", TerminalName},
            {"output-log", OutputLogName},
            {"outputlog", OutputLogName},
            {"mediaplay", MediaPlayName},
            {"media-play", MediaPlayName},
            {"profiler", ProfilerName},
            {"list", ListName},
            {"refresh", RefreshName},
            {"refresh-cw", RefreshName},
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
            {"blueprint", BlocksName},
            {"static_mesh", CubeName},
            {"skeletal_mesh", CubeName},
            {"audio", Volume2Name},
            {"font", DocumentName},
            {"script", CodeName},
            {"scene", MapName},
            {"level", MapName},
            {"animation", MediaPlayName},
            {"particle", SparklesName},
            {"collection", LayersName},
            {"plugin", PackageName},
            {"engine", BuildName},
            {"project", ProjectFolderName},
            {"favorites", StarName},
            {"trash-2", DeleteName},
            {"clipboard", PasteName},
            {"alert-triangle", WarningName},
            {"x-circle", ErrorName},
            {"folder-kanban", ProjectFolderName},
            {"blocks", BlocksName},
            {"sliders-horizontal", PropertiesName},
            {"activity", ProfilerName},
            {"pause", StopName},
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
