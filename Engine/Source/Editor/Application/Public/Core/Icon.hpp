#pragma once

#include "Geometry.hpp"
#include <string>
#include <memory>
#include <unordered_map>

namespace we::UI {

class PaintContext;

// Icon registry with SVG path data
class IconRegistry {
public:
    static IconRegistry& Get();
    
    // Register an icon with its SVG path data
    void RegisterIcon(const std::string& name, const std::string& svgPath) {
        m_Icons[name] = svgPath;
    }
    
    // Get SVG path data for an icon
    std::string GetIconPath(const std::string& name) const {
        auto it = m_Icons.find(name);
        if (it != m_Icons.end()) {
            return it->second;
        }
        return ""; // Not found
    }
    
    // Check if icon exists
    bool HasIcon(const std::string& name) const {
        return m_Icons.find(name) != m_Icons.end();
    }
    
    // Initialize with default icon set
    void InitializeDefaultIcons();
    
private:
    IconRegistry() = default;
    std::unordered_map<std::string, std::string> m_Icons;
};

// Icon widget for rendering icons
class IconPainter {
public:
    // Draw an icon at the specified position with size and color (by name)
    static void DrawIcon(PaintContext& context, const std::string& iconName, 
                        const Point& position, float size, const Color& color);
    
    // Draw an icon with custom transform (by name)
    static void DrawIcon(PaintContext& context, const std::string& iconName,
                        const Rect& bounds, const Color& color);
    
    // Draw an icon by codepoint at position
    static void DrawIcon(PaintContext& context, int codepoint, const Rect& bounds, const Color& color);
};

// Material Icons (Classic) Unicode codepoints
// Full reference: https://fonts.google.com/icons
namespace Icons {
    // Selection tools
    constexpr int Cursor     = 0xEA8C; // pointer
    constexpr int Move       = 0xEB22; // move
    constexpr int Rotate     = 0xEB37; // refresh
    constexpr int Scale      = 0xEB4C; // screen-full

    // Playback
    constexpr int Play       = 0xEB2C; // play
    constexpr int Pause      = 0xEAD1; // debug-pause
    constexpr int Stop       = 0xEAD7; // debug-stop

    // View modes
    constexpr int Perspective = 0xEB29; // package
    constexpr int Lit        = 0xEA61; // lightbulb
    constexpr int Wireframe  = 0xEB01; // globe

    // Camera
    constexpr int Camera     = 0xEADA; // device-camera

    // Tools
    constexpr int Snap       = 0xEBAE; // magnet
    constexpr int Grid       = 0xEBEB; // layout

    // Objects
    constexpr int Cube       = 0xEB29; // package
    constexpr int Sphere     = 0xEB01; // globe
    constexpr int Plane      = 0xEABA; // chrome-minimize
    constexpr int Cylinder   = 0xEACE; // database

    // Lighting
    constexpr int Light      = 0xEA61; // lightbulb
    constexpr int Sun        = 0xEA61; // lightbulb
    constexpr int PointLight = 0xEB13; // lightbulb-autofix

    // Materials
    constexpr int Material   = 0xEB2A; // paintcan
    constexpr int Shader     = 0xEAC4; // code
    constexpr int Texture    = 0xEAEA; // file-media

    // File operations
    constexpr int Save       = 0xEB4B; // save
    constexpr int Open       = 0xEAF7; // folder-opened
    constexpr int New        = 0xEA7F; // new-file
    constexpr int Folder     = 0xEA83; // folder
    constexpr int Document   = 0xEA7B; // file
    
    // Compilation/Build
    constexpr int Code       = 0xEAC4; // code
    constexpr int Build      = 0xEB65; // wrench
    constexpr int Package    = 0xEB29; // package

    // Edit
    constexpr int Undo       = 0xEAE2; // discard
    constexpr int Redo       = 0xEBB0; // redo
    constexpr int Copy       = 0xEBCC; // copy
    constexpr int Paste      = 0xEBA4; // pass
    constexpr int Delete     = 0xEA81; // trash

    // UI
    constexpr int Search     = 0xEA6D; // search
    constexpr int Settings   = 0xEB51; // settings-gear
    constexpr int Menu       = 0xEB94; // menu
    constexpr int More       = 0xEA7C; // ellipsis

    // Tree
    constexpr int ChevronRight = 0xEAB6; // chevron-right
    constexpr int ChevronDown = 0xEAB4; // chevron-down
    constexpr int ChevronLeft = 0xEAB5; // chevron-left

    // Visibility
    constexpr int Eye        = 0xEA70; // eye
    constexpr int EyeOff     = 0xEAE7; // eye-closed
    constexpr int Lock       = 0xEA75; // lock
    constexpr int Unlock     = 0xEB74; // unlock

    // Panels
    constexpr int Layers     = 0xEB78; // versions
    constexpr int Hierarchy  = 0xEBB9; // type-hierarchy
    constexpr int Properties = 0xEB51; // settings-gear
    constexpr int Console    = 0xEA85; // terminal
    constexpr int Profiler   = 0xEB03; // graph

    // Content browser
    constexpr int List       = 0xEB84; // list-flat
    constexpr int Refresh    = 0xEB37; // refresh
    constexpr int Star       = 0xEA6A; // star-empty
    constexpr int StarFilled = 0xEB59; // star-full

    // Misc
    constexpr int Plus       = 0xEA60; // add
    constexpr int Minus      = 0xEB3B; // remove
    constexpr int X          = 0xEA76; // close
    constexpr int Check      = 0xEAB2; // check
    constexpr int Info       = 0xEA74; // info
    constexpr int Warning    = 0xEA6C; // warning
    constexpr int Error      = 0xEA87; // error
    constexpr int Success    = 0xEAB2; // check

    // Window Controls
    constexpr int Minimize   = 0xEABA; // chrome-minimize
    constexpr int Maximize   = 0xEAB9; // chrome-maximize
    constexpr int Restore    = 0xEABB; // chrome-restore

    // String-name aliases (for backwards compatibility with AddTool calls)
    constexpr const char* CursorName     = "cursor";
    constexpr const char* MoveName       = "move";
    constexpr const char* RotateName     = "rotate";
    constexpr const char* ScaleName      = "scale";
    constexpr const char* PlayName       = "play";
    constexpr const char* PauseName      = "pause";
    constexpr const char* StopName       = "stop";
    constexpr const char* PerspectiveName = "perspective";
    constexpr const char* LitName        = "lit";
    constexpr const char* WireframeName  = "wireframe";
    constexpr const char* CameraName     = "camera";
    constexpr const char* SnapName       = "snap";
    constexpr const char* GridName       = "grid";
    constexpr const char* CubeName       = "cube";
    constexpr const char* SphereName     = "sphere";
    constexpr const char* PlaneName      = "plane";
    constexpr const char* CylinderName   = "cylinder";
    constexpr const char* LightName      = "light";
    constexpr const char* SunName        = "sun";
    constexpr const char* PointLightName = "point-light";
    constexpr const char* MaterialName   = "material";
    constexpr const char* ShaderName     = "shader";
    constexpr const char* TextureName    = "texture";
    constexpr const char* SaveName       = "save";
    constexpr const char* OpenName       = "open";
    constexpr const char* NewName        = "new";
    constexpr const char* FolderName     = "folder";
    constexpr const char* DocumentName   = "document";
    constexpr const char* CodeName       = "code";
    constexpr const char* BuildName      = "build";
    constexpr const char* PackageName    = "package";
    constexpr const char* UndoName       = "undo";
    constexpr const char* RedoName       = "redo";
    constexpr const char* CopyName       = "copy";
    constexpr const char* PasteName      = "paste";
    constexpr const char* DeleteName     = "delete";
    constexpr const char* SearchName     = "search";
    constexpr const char* SettingsName   = "settings";
    constexpr const char* MenuName       = "menu";
    constexpr const char* MoreName       = "more";
    constexpr const char* ChevronRightName = "chevron-right";
    constexpr const char* ChevronDownName  = "chevron-down";
    constexpr const char* ChevronLeftName  = "chevron-left";
    constexpr const char* EyeName        = "eye";
    constexpr const char* EyeOffName     = "eye-off";
    constexpr const char* LockName       = "lock";
    constexpr const char* UnlockName     = "unlock";
    constexpr const char* LayersName     = "layers";
    constexpr const char* HierarchyName  = "hierarchy";
    constexpr const char* PropertiesName = "properties";
    constexpr const char* ConsoleName    = "console";
    constexpr const char* ProfilerName   = "profiler";
    constexpr const char* ListName       = "list";
    constexpr const char* RefreshName    = "refresh";
    constexpr const char* StarName       = "star";
    constexpr const char* StarFilledName = "star-filled";
    constexpr const char* PlusName       = "plus";
    constexpr const char* MinusName      = "minus";
    constexpr const char* XName          = "x";
    constexpr const char* CheckName      = "check";
    constexpr const char* InfoName       = "info";
    constexpr const char* WarningName    = "warning";
    constexpr const char* ErrorName      = "error";
    constexpr const char* SuccessName    = "success";

    constexpr const char* MinimizeName   = "minimize";
    constexpr const char* MaximizeName   = "maximize";
    constexpr const char* RestoreName    = "restore";

    // Get Material Icons codepoint by string name
    inline int GetCodepoint(const std::string& name) {
        if (name == "cursor")        return Cursor;
        if (name == "move")          return Move;
        if (name == "rotate")        return Rotate;
        if (name == "scale")         return Scale;
        if (name == "play")          return Play;
        if (name == "pause")         return Pause;
        if (name == "stop")          return Stop;
        if (name == "lit")           return Lit;
        if (name == "wireframe")     return Wireframe;
        if (name == "camera")        return Camera;
        if (name == "cube")          return Cube;
        if (name == "sphere")        return Sphere;
        if (name == "plane")         return Plane;
        if (name == "light")         return Light;
        if (name == "sun")           return Sun;
        if (name == "folder")        return Folder;
        if (name == "save")          return Save;
        if (name == "open")          return Open;
        if (name == "new")           return New;
        if (name == "document")      return Document;
        if (name == "code")          return Code;
        if (name == "build")         return Build;
        if (name == "package")       return Package;
        if (name == "undo")          return Undo;
        if (name == "redo")          return Redo;
        if (name == "delete")        return Delete;
        if (name == "search")        return Search;
        if (name == "settings")      return Settings;
        if (name == "menu")          return Menu;
        if (name == "more")          return More;
        if (name == "chevron-right") return ChevronRight;
        if (name == "chevron-down")  return ChevronDown;
        if (name == "chevron-left")  return ChevronLeft;
        if (name == "eye")           return Eye;
        if (name == "eye-off")       return EyeOff;
        if (name == "lock")          return Lock;
        if (name == "unlock")        return Unlock;
        if (name == "layers")        return Layers;
        if (name == "list")          return List;
        if (name == "refresh")       return Refresh;
        if (name == "hierarchy")     return Hierarchy;
        if (name == "properties")    return Properties;
        if (name == "console")       return Console;
        if (name == "profiler")      return Profiler;
        if (name == "perspective")   return Perspective;
        if (name == "material")      return Material;
        if (name == "shader")        return Shader;
        if (name == "texture")       return Texture;
        if (name == "package")       return Package;
        if (name == "plus")          return Plus;
        if (name == "minus")         return Minus;
        if (name == "x")             return X;
        if (name == "check")         return Check;
        if (name == "info")          return Info;
        if (name == "warning")       return Warning;
        if (name == "error")         return Error;
        if (name == "success")       return Success;
        if (name == "minimize")      return Minimize;
        if (name == "maximize")      return Maximize;
        if (name == "restore")       return Restore;
        return 0xE001; // fallback: generic error icon
    }
}

} // namespace we::editor::application::UI
