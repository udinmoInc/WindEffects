#pragma once

// Canonical HLSL shader asset names (bytecode: {Name}_VS.spv / {Name}_PS.spv / {Name}_CS.spv).
namespace we::runtime::renderer::shaders {

inline constexpr const char* EditorBackground = "EditorBackground";
inline constexpr const char* EditorGrid = "EditorGrid";
inline constexpr const char* SceneObject = "SceneObject";
inline constexpr const char* UI = "UI";

// Planned rendering passes (sources under Engine/Shaders/).
inline constexpr const char* DeferredGBuffer = "DeferredGBuffer";
inline constexpr const char* ForwardPlusLighting = "ForwardPlusLighting";
inline constexpr const char* ClusteredLightCulling = "ClusteredLightCulling";
inline constexpr const char* PostProcessComposite = "PostProcessComposite";

} // namespace we::runtime::renderer::shaders
