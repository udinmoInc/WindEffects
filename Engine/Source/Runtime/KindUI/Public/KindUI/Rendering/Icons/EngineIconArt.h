#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"
#include "KindUI/Core/PaintContext.h"

#include <string>
#include <string_view>
#include <unordered_map>

#include "RHI/Types.h"

namespace we::runtime::kindui {

class IconRenderer;

namespace EngineIcons {

constexpr const char* kPrefix = "we/";

// Primitives
constexpr const char* Cube = "we/cube";
constexpr const char* MeshCube = "mesh-cube";
constexpr const char* Sphere = "we/sphere";
constexpr const char* Capsule = "we/capsule";
constexpr const char* Cylinder = "we/cylinder";
constexpr const char* Cone = "we/cone";
constexpr const char* Plane = "we/plane";
constexpr const char* Torus = "we/torus";
constexpr const char* Pyramid = "we/pyramid";
constexpr const char* Arrow = "we/arrow";

// Lights
constexpr const char* LightPoint = "we/light-point";
constexpr const char* LightSpot = "we/light-spot";
constexpr const char* LightDirectional = "we/light-directional";
constexpr const char* LightRect = "we/light-rect";
constexpr const char* LightSky = "we/light-sky";

// Cameras
constexpr const char* Camera = "we/camera";
constexpr const char* CineCamera = "we/cine-camera";

// Environment
constexpr const char* Sky = "we/sky";
constexpr const char* Landscape = "we/landscape";
constexpr const char* Terrain = "we/terrain";
constexpr const char* Water = "we/water";
constexpr const char* Clouds = "we/clouds";
constexpr const char* Fog = "we/fog";
constexpr const char* Foliage = "we/foliage";

// Effects
constexpr const char* Particle = "we/particle";
constexpr const char* Effect = "we/effect";
constexpr const char* Decal = "we/decal";
constexpr const char* ReflectionProbe = "we/reflection-probe";

// Gameplay
constexpr const char* Character = "we/character";
constexpr const char* Pawn = "we/pawn";
constexpr const char* EmptyActor = "we/empty-actor";
constexpr const char* Blueprint = "we/blueprint";
constexpr const char* TriggerVolume = "we/trigger-volume";
constexpr const char* PhysicsVolume = "we/physics-volume";
constexpr const char* NavVolume = "we/nav-volume";

// Assets
constexpr const char* Material = "we/material";
constexpr const char* Texture = "we/texture";
constexpr const char* StaticMesh = "we/static-mesh";
constexpr const char* SkeletalMesh = "we/skeletal-mesh";
constexpr const char* Animation = "we/animation";
constexpr const char* Audio = "we/audio";
constexpr const char* Font = "we/font";
constexpr const char* Folder = "we/folder";
constexpr const char* Scene = "we/scene";
constexpr const char* Level = "we/level";

// Editor
constexpr const char* Search = "we/search";
constexpr const char* Settings = "we/settings";
constexpr const char* Save = "we/save";
constexpr const char* Undo = "we/undo";
constexpr const char* Redo = "we/redo";
constexpr const char* Refresh = "we/refresh";
constexpr const char* Delete = "we/delete";
constexpr const char* Duplicate = "we/duplicate";
constexpr const char* Rename = "we/rename";
constexpr const char* Import = "we/import";
constexpr const char* Export = "we/export";
constexpr const char* Play = "we/play";
constexpr const char* Pause = "we/pause";
constexpr const char* Stop = "we/stop";
constexpr const char* Filter = "we/filter";
constexpr const char* Star = "we/star";
constexpr const char* Note = "we/note";
constexpr const char* Widget = "we/widget";
constexpr const char* AI = "we/ai";
constexpr const char* Navigation = "we/navigation";
constexpr const char* Trigger = "we/trigger";
constexpr const char* CategoryBasic = "we/category-basic";
constexpr const char* CategoryGeometry = "we/category-geometry";
constexpr const char* CategoryLights = "we/category-lights";
constexpr const char* CategoryAll = "we/category-all";

[[nodiscard]] bool IsEngineIcon(std::string_view iconName);

} // namespace EngineIcons

class KINDUI_API EngineIconArt {
public:
    static EngineIconArt& Get();

    void Initialize(IconRenderer* renderer);
    void InvalidateCache();

    [[nodiscard]] bool IsEngineIcon(std::string_view iconName) const;

    [[nodiscard]] we::rhi::RHIDescriptorSetHandle GetTexture(
        std::string_view iconName,
        uint32_t displaySizePx,
        float hoverAnim = 0.0f,
        float activeAnim = 0.0f) const;

    void Paint(
        PaintContext& context,
        std::string_view iconName,
        const Rect& bounds,
        float hoverAnim = 0.0f,
        float activeAnim = 0.0f) const;

private:
    IconRenderer* m_Renderer = nullptr;
    mutable std::unordered_map<std::string, we::rhi::RHIDescriptorSetHandle> m_Cache;
};

} // namespace we::runtime::kindui
