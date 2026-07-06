#pragma once

// Canonical HLSL shader asset names (bytecode: {Name}_VS.spv / {Name}_PS.spv / {Name}_CS.spv).
namespace we::runtime::renderer::shaders {

inline constexpr const char* AtmospherePass = "AtmospherePass";
inline constexpr const char* VolumetricCloudsPass = "VolumetricCloudsPass";
inline constexpr const char* CloudTemporalResolve = "CloudTemporalResolve";
inline constexpr const char* CloudCompositePass = "CloudCompositePass";
inline constexpr const char* FogCompositePass = "FogCompositePass";
inline constexpr const char* EditorGrid = "EditorGrid";
inline constexpr const char* SceneObject = "SceneObject";
inline constexpr const char* UI = "UI";
inline constexpr const char* PostExposurePass = "PostExposurePass";
inline constexpr const char* PostExposureCS = "PostExposureCS";
inline constexpr const char* PostCompositeCS = "PostCompositeCS";
inline constexpr const char* LuminanceReduceCS = "LuminanceReduceCS";
inline constexpr const char* LuminanceAvgCS = "LuminanceAvgCS";
inline constexpr const char* BloomPrefilterCS = "BloomPrefilterCS";
inline constexpr const char* BloomBlurCS = "BloomBlurCS";

// Planned rendering passes (sources under Engine/Shaders/).
inline constexpr const char* DeferredGBuffer = "DeferredGBuffer";
inline constexpr const char* ForwardPlusLighting = "ForwardPlusLighting";
inline constexpr const char* ClusteredLightCulling = "ClusteredLightCulling";
inline constexpr const char* PostProcessComposite = "PostProcessComposite";

} // namespace we::runtime::renderer::shaders
