#pragma once

#include "Renderer/Export.hpp"

#if WE_HAS_VULKAN
#include <volk.h>
#endif

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

class VulkanContext;

// Sequential validation stages for atmosphere / post-process debugging.
enum class AtmosphereValidationStage : int {
    None = 0,
    ConstantRed = 1,
    ConstantGreen = 2,
    ConstantBlue = 3,
    ViewDirection = 4,
    SunDirection = 5,
    RayleighOnly = 6,
    MieOnly = 7,
    TransmittanceOnly = 8,
    OpticalDepthOnly = 9,
    SunDiskOnly = 10,
    Count = 11,
};

struct AtmosphereValidationSettings {
    bool enabled = false;
    bool autoAdvance = true;
    bool exitOnComplete = true;
    bool exitOnFailure = true;
    int warmupFrames = 3;
    AtmosphereValidationStage startStage = AtmosphereValidationStage::ConstantRed;
};

struct AtmosphereValidationPixel {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};

struct AtmosphereValidationResult {
    AtmosphereValidationStage stage = AtmosphereValidationStage::None;
    bool passed = false;
    std::string message;
    AtmosphereValidationPixel center;
    std::string screenshotPath;
};

class AtmosphereValidation {
public:
    RENDERER_API static AtmosphereValidation& Get();

    RENDERER_API void Configure(const AtmosphereValidationSettings& settings);
    RENDERER_API bool IsActive() const { return m_Settings.enabled; }
    RENDERER_API AtmosphereValidationStage GetCurrentStage() const { return m_CurrentStage; }
    RENDERER_API int GetShaderDebugMode() const;

    RENDERER_API void ApplyEnvironmentOverrides(struct SceneEnvironmentUniform& uniform) const;

    // Call after post-process each frame while validation is active.
    RENDERER_API bool OnFrameComplete(
        const VulkanContext& context,
        VkImage colorImage,
        uint32_t width,
        uint32_t height);

    RENDERER_API bool ShouldExit() const { return m_ShouldExit; }
    RENDERER_API const std::vector<AtmosphereValidationResult>& GetResults() const { return m_Results; }

    RENDERER_API static const char* StageName(AtmosphereValidationStage stage);
    RENDERER_API static AtmosphereValidationSettings ParseCommandLine(int argc, char* argv[]);

private:
    AtmosphereValidation() = default;

    bool ReadbackImage(
        const VulkanContext& context,
        VkImage image,
        uint32_t width,
        uint32_t height,
        std::vector<AtmosphereValidationPixel>& outPixels) const;

    bool SavePpm(
        const std::string& path,
        uint32_t width,
        uint32_t height,
        const std::vector<AtmosphereValidationPixel>& pixels) const;

    AtmosphereValidationResult ValidateStage(
        AtmosphereValidationStage stage,
        uint32_t width,
        uint32_t height,
        const std::vector<AtmosphereValidationPixel>& pixels) const;

    void AdvanceStage();

    AtmosphereValidationSettings m_Settings{};
    AtmosphereValidationStage m_CurrentStage = AtmosphereValidationStage::ConstantRed;
    int m_WarmupRemaining = 0;
    bool m_ShouldExit = false;
    std::vector<AtmosphereValidationResult> m_Results;
};

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
