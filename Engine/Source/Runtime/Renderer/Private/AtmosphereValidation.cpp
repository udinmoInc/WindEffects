#include "Renderer/AtmosphereValidation.hpp"
#include "Renderer/SceneEnvironmentUniform.hpp"
#include "Renderer/VulkanContext.hpp"
#include "Renderer/RendererConfig.hpp"
#include "Core/LogCategory.hpp"
#include "Core/DiagnosticMacros.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

namespace {

float HalfToFloat(std::uint16_t value) {
    const std::uint32_t sign = (value >> 15) & 0x1;
    const std::uint32_t exponent = (value >> 10) & 0x1F;
    const std::uint32_t mantissa = value & 0x3FF;

    if (exponent == 0) {
        if (mantissa == 0) {
            return sign ? -0.0f : 0.0f;
        }
        const float result = std::ldexp(static_cast<float>(mantissa), -24);
        return sign ? -result : result;
    }
    if (exponent == 31) {
        return mantissa == 0 ? (sign ? -INFINITY : INFINITY) : NAN;
    }

    const std::uint32_t floatBits =
        (sign << 31)
        | ((exponent + (127 - 15)) << 23)
        | (mantissa << 13);
    float result = 0.0f;
    std::memcpy(&result, &floatBits, sizeof(result));
    return result;
}

int StageToShaderDebugMode(AtmosphereValidationStage stage) {
    switch (stage) {
        case AtmosphereValidationStage::ConstantRed: return 101;
        case AtmosphereValidationStage::ConstantGreen: return 102;
        case AtmosphereValidationStage::ConstantBlue: return 103;
        case AtmosphereValidationStage::ViewDirection: return 1;
        case AtmosphereValidationStage::SunDirection: return 2;
        case AtmosphereValidationStage::RayleighOnly: return 6;
        case AtmosphereValidationStage::MieOnly: return 7;
        case AtmosphereValidationStage::TransmittanceOnly: return 5;
        case AtmosphereValidationStage::OpticalDepthOnly: return 4;
        case AtmosphereValidationStage::SunDiskOnly: return 9;
        default: return 0;
    }
}

bool StartsWith(const char* value, const char* prefix) {
    return value != nullptr && prefix != nullptr && std::strncmp(value, prefix, std::strlen(prefix)) == 0;
}

float PixelDistance(const AtmosphereValidationPixel& a, const AtmosphereValidationPixel& b) {
    const float dr = a.r - b.r;
    const float dg = a.g - b.g;
    const float db = a.b - b.b;
    return std::sqrt(dr * dr + dg * dg + db * db);
}

} // namespace

AtmosphereValidation& AtmosphereValidation::Get() {
    static AtmosphereValidation instance;
    return instance;
}

void AtmosphereValidation::Configure(const AtmosphereValidationSettings& settings) {
    m_Settings = settings;
    m_CurrentStage = settings.startStage;
    m_WarmupRemaining = settings.warmupFrames;
    m_ShouldExit = false;
    m_Results.clear();

    if (m_Settings.enabled) {
        WE_LOG_INFO(
            we::runtime::core::LogCategory::Renderer.data(),
            std::string("Atmosphere validation enabled — starting at stage ")
                + StageName(m_CurrentStage));
    }
}

int AtmosphereValidation::GetShaderDebugMode() const {
    if (!m_Settings.enabled) {
        return 0;
    }
    return StageToShaderDebugMode(m_CurrentStage);
}

void AtmosphereValidation::ApplyEnvironmentOverrides(SceneEnvironmentUniform& uniform) const {
    if (!m_Settings.enabled) {
        return;
    }

    uniform.atmosphereDebugMode = GetShaderDebugMode();
    uniform.enableClouds = 0.0f;
    uniform.enableVolumetricFog = 0.0f;
    uniform.bloomIntensity = 0.0f;
    uniform.enableAutoExposure = 0.0f;
    uniform.exposureEV = 0.0f;
    uniform.exposureCompensation = 0.0f;
}

const char* AtmosphereValidation::StageName(AtmosphereValidationStage stage) {
    switch (stage) {
        case AtmosphereValidationStage::ConstantRed: return "ConstantRed";
        case AtmosphereValidationStage::ConstantGreen: return "ConstantGreen";
        case AtmosphereValidationStage::ConstantBlue: return "ConstantBlue";
        case AtmosphereValidationStage::ViewDirection: return "ViewDirection";
        case AtmosphereValidationStage::SunDirection: return "SunDirection";
        case AtmosphereValidationStage::RayleighOnly: return "RayleighOnly";
        case AtmosphereValidationStage::MieOnly: return "MieOnly";
        case AtmosphereValidationStage::TransmittanceOnly: return "TransmittanceOnly";
        case AtmosphereValidationStage::OpticalDepthOnly: return "OpticalDepthOnly";
        case AtmosphereValidationStage::SunDiskOnly: return "SunDiskOnly";
        default: return "None";
    }
}

AtmosphereValidationSettings AtmosphereValidation::ParseCommandLine(int argc, char* argv[]) {
    AtmosphereValidationSettings settings{};

    if (const char* env = std::getenv("WE_ATMOSPHERE_VALIDATION")) {
        if (env[0] == '1' || std::strcmp(env, "true") == 0 || std::strcmp(env, "TRUE") == 0) {
            settings.enabled = true;
            settings.autoAdvance = true;
            settings.exitOnComplete = true;
            settings.exitOnFailure = true;
        }
    }

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-AtmosphereValidation") == 0
            || std::strcmp(argv[i], "--atmosphere-validation") == 0) {
            settings.enabled = true;
            settings.autoAdvance = true;
            settings.exitOnComplete = true;
            settings.exitOnFailure = true;
            continue;
        }

        if (StartsWith(argv[i], "-AtmosphereValidation=")) {
            const char* value = argv[i] + std::strlen("-AtmosphereValidation=");
            settings.enabled = true;
            if (std::strcmp(value, "manual") == 0) {
                settings.autoAdvance = false;
                settings.exitOnComplete = false;
            }
            continue;
        }

        if (StartsWith(argv[i], "-AtmosphereValidationStage=")) {
            settings.enabled = true;
            const int stage = std::atoi(argv[i] + std::strlen("-AtmosphereValidationStage="));
            settings.startStage = static_cast<AtmosphereValidationStage>(std::clamp(stage, 1, 10));
            settings.autoAdvance = false;
            settings.exitOnComplete = false;
        }
    }
    return settings;
}

bool AtmosphereValidation::ReadbackImage(
    const VulkanContext& context,
    VkImage image,
    uint32_t width,
    uint32_t height,
    std::vector<AtmosphereValidationPixel>& outPixels) const {

    const VkDevice device = context.GetDevice();
    const VkDeviceSize bytesPerPixel = 8;
    const VkDeviceSize rowPitch = ((width * bytesPerPixel + 255) / 256) * 256;
    const VkDeviceSize bufferSize = rowPitch * height;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    context.CreateBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

    VkCommandBuffer cmd = context.BeginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    context.TransitionImageLayout(image, kOffscreenColorFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &region);
    context.TransitionImageLayout(image, kOffscreenColorFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    context.EndSingleTimeCommands(cmd);

    void* mapped = nullptr;
    vkMapMemory(device, stagingMemory, 0, bufferSize, 0, &mapped);

    outPixels.resize(static_cast<size_t>(width) * height);
    for (uint32_t y = 0; y < height; ++y) {
        const auto* row = reinterpret_cast<const std::uint8_t*>(mapped) + y * rowPitch;
        for (uint32_t x = 0; x < width; ++x) {
            const auto* pixel = row + x * bytesPerPixel;
            const auto r16 = static_cast<std::uint16_t>(pixel[0] | (pixel[1] << 8));
            const auto g16 = static_cast<std::uint16_t>(pixel[2] | (pixel[3] << 8));
            const auto b16 = static_cast<std::uint16_t>(pixel[4] | (pixel[5] << 8));

            auto& out = outPixels[static_cast<size_t>(y) * width + x];
            out.r = HalfToFloat(r16);
            out.g = HalfToFloat(g16);
            out.b = HalfToFloat(b16);
        }
    }

    vkUnmapMemory(device, stagingMemory);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
    return true;
}

bool AtmosphereValidation::SavePpm(
    const std::string& path,
    uint32_t width,
    uint32_t height,
    const std::vector<AtmosphereValidationPixel>& pixels) const {

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    file << "P6\n" << width << " " << height << "\n255\n";
    for (const auto& pixel : pixels) {
        const auto clampByte = [](float value) -> unsigned char {
            return static_cast<unsigned char>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
        };
        const unsigned char bytes[3] = { clampByte(pixel.r), clampByte(pixel.g), clampByte(pixel.b) };
        file.write(reinterpret_cast<const char*>(bytes), 3);
    }
    return file.good();
}

AtmosphereValidationResult AtmosphereValidation::ValidateStage(
    AtmosphereValidationStage stage,
    uint32_t width,
    uint32_t height,
    const std::vector<AtmosphereValidationPixel>& pixels) const {

    AtmosphereValidationResult result{};
    result.stage = stage;

    if (pixels.empty() || width == 0 || height == 0) {
        result.passed = false;
        result.message = "Readback returned no pixels.";
        return result;
    }

    const size_t centerIndex = static_cast<size_t>(height / 2) * width + (width / 2);
    const size_t leftIndex = static_cast<size_t>(height / 2) * width + (width / 4);
    const size_t rightIndex = static_cast<size_t>(height / 2) * width + ((width * 3) / 4);
    result.center = pixels[centerIndex];

    std::ostringstream detail;
    detail << "center=(" << result.center.r << ", " << result.center.g << ", " << result.center.b << ")";

    auto expectSolid = [&](float expectedR, float expectedG, float expectedB, float tolerance) {
        const float dr = std::abs(result.center.r - expectedR);
        const float dg = std::abs(result.center.g - expectedG);
        const float db = std::abs(result.center.b - expectedB);
        if (dr > tolerance || dg > tolerance || db > tolerance) {
            result.passed = false;
            result.message = "Expected solid color (" + std::to_string(expectedR) + ", "
                + std::to_string(expectedG) + ", " + std::to_string(expectedB) + ") but got " + detail.str();
            return;
        }

        const AtmosphereValidationPixel& left = pixels[leftIndex];
        const AtmosphereValidationPixel& right = pixels[rightIndex];
        if (PixelDistance(left, result.center) > tolerance || PixelDistance(right, result.center) > tolerance) {
            result.passed = false;
            result.message = "Expected uniform color across viewport but corners differ. " + detail.str();
            return;
        }

        result.passed = true;
        result.message = "Solid color matched. " + detail.str();
    };

    switch (stage) {
        case AtmosphereValidationStage::ConstantRed:
            expectSolid(1.0f, 0.0f, 0.0f, 0.02f);
            break;
        case AtmosphereValidationStage::ConstantGreen:
            expectSolid(0.0f, 1.0f, 0.0f, 0.02f);
            break;
        case AtmosphereValidationStage::ConstantBlue:
            expectSolid(0.0f, 0.0f, 1.0f, 0.02f);
            break;
        case AtmosphereValidationStage::ViewDirection: {
            const AtmosphereValidationPixel& left = pixels[leftIndex];
            const AtmosphereValidationPixel& right = pixels[rightIndex];
            const float variation = PixelDistance(left, right);
            if (variation < 0.05f) {
                result.passed = false;
                result.message = "View-direction debug should vary across the viewport. " + detail.str();
            } else {
                result.passed = true;
                result.message = "View-direction varies across viewport (delta=" + std::to_string(variation) + "). " + detail.str();
            }
            break;
        }
        case AtmosphereValidationStage::SunDirection: {
            const AtmosphereValidationPixel& left = pixels[leftIndex];
            const AtmosphereValidationPixel& right = pixels[rightIndex];
            const float variation = PixelDistance(left, right);
            if (variation > 0.02f) {
                result.passed = false;
                result.message = "Sun-direction debug should be uniform across the viewport. " + detail.str();
            } else if (result.center.r < 0.05f && result.center.g < 0.05f && result.center.b < 0.05f) {
                result.passed = false;
                result.message = "Sun-direction debug is near black. " + detail.str();
            } else {
                result.passed = true;
                result.message = "Sun-direction is uniform. " + detail.str();
            }
            break;
        }
        case AtmosphereValidationStage::RayleighOnly:
        case AtmosphereValidationStage::MieOnly:
        case AtmosphereValidationStage::TransmittanceOnly:
        case AtmosphereValidationStage::OpticalDepthOnly: {
            float minR = 1.0f, minG = 1.0f, minB = 1.0f;
            float maxR = 0.0f, maxG = 0.0f, maxB = 0.0f;
            for (const auto& pixel : pixels) {
                minR = std::min(minR, pixel.r);
                minG = std::min(minG, pixel.g);
                minB = std::min(minB, pixel.b);
                maxR = std::max(maxR, pixel.r);
                maxG = std::max(maxG, pixel.g);
                maxB = std::max(maxB, pixel.b);
            }
            const float range = std::max({ maxR - minR, maxG - minG, maxB - minB });
            if (range < 0.01f && result.center.r < 0.01f && result.center.g < 0.01f && result.center.b < 0.01f) {
                result.passed = false;
                result.message = "Stage output is flat black. " + detail.str();
            } else {
                result.passed = true;
                result.message = "Stage produced non-flat signal (range=" + std::to_string(range) + "). " + detail.str();
            }
            break;
        }
        case AtmosphereValidationStage::SunDiskOnly: {
            float maxLum = 0.0f;
            size_t brightCount = 0;
            for (const auto& pixel : pixels) {
                const float lum = pixel.r + pixel.g + pixel.b;
                maxLum = std::max(maxLum, lum);
                if (lum > 0.2f) {
                    ++brightCount;
                }
            }
            const float brightRatio = static_cast<float>(brightCount) / static_cast<float>(pixels.size());
            if (maxLum < 0.15f) {
                result.passed = false;
                result.message = "Sun disk not visible (max luminance too low). " + detail.str();
            } else if (brightRatio > 0.5f) {
                result.passed = false;
                result.message = "Too much of the viewport is bright; expected a localized sun disk. " + detail.str();
            } else {
                result.passed = true;
                result.message = "Localized bright sun disk detected. " + detail.str();
            }
            break;
        }
        default:
            result.passed = false;
            result.message = "Unknown validation stage.";
            break;
    }

    return result;
}

void AtmosphereValidation::AdvanceStage() {
    const int next = static_cast<int>(m_CurrentStage) + 1;
    if (next >= static_cast<int>(AtmosphereValidationStage::Count)) {
        m_ShouldExit = m_Settings.exitOnComplete;
        m_Settings.enabled = false;
        WE_LOG_INFO(we::runtime::core::LogCategory::Renderer.data(), "Atmosphere validation completed all stages.");
        return;
    }

    m_CurrentStage = static_cast<AtmosphereValidationStage>(next);
    m_WarmupRemaining = m_Settings.warmupFrames;
    WE_LOG_INFO(
        we::runtime::core::LogCategory::Renderer.data(),
        std::string("Atmosphere validation advancing to stage ") + StageName(m_CurrentStage));
}

bool AtmosphereValidation::OnFrameComplete(
    const VulkanContext& context,
    VkImage colorImage,
    uint32_t width,
    uint32_t height) {

    if (!m_Settings.enabled || m_ShouldExit) {
        return false;
    }

    if (m_WarmupRemaining > 0) {
        --m_WarmupRemaining;
        return false;
    }

    std::vector<AtmosphereValidationPixel> pixels;
    context.WaitUntilIdle();
    if (!ReadbackImage(context, colorImage, width, height, pixels)) {
        AtmosphereValidationResult failed{};
        failed.stage = m_CurrentStage;
        failed.passed = false;
        failed.message = "GPU readback failed.";
        m_Results.push_back(failed);
        WE_LOG_ERROR(we::runtime::core::LogCategory::Renderer.data(), failed.message);
        if (m_Settings.exitOnFailure) {
            m_ShouldExit = true;
            m_Settings.enabled = false;
        }
        return true;
    }

    const std::filesystem::path outputDir = std::filesystem::path("Saved") / "Validation";
    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);

    AtmosphereValidationResult result = ValidateStage(m_CurrentStage, width, height, pixels);
    result.screenshotPath = (outputDir / (std::string(StageName(m_CurrentStage)) + ".ppm")).string();
    SavePpm(result.screenshotPath, width, height, pixels);
    m_Results.push_back(result);

    if (result.passed) {
        WE_LOG_INFO(
            we::runtime::core::LogCategory::Renderer.data(),
            std::string("PASS [") + StageName(m_CurrentStage) + "]: " + result.message
                + " | screenshot=" + result.screenshotPath);
    } else {
        WE_LOG_ERROR(
            we::runtime::core::LogCategory::Renderer.data(),
            std::string("FAIL [") + StageName(m_CurrentStage) + "]: " + result.message
                + " | screenshot=" + result.screenshotPath);
        if (m_Settings.exitOnFailure) {
            m_ShouldExit = true;
            m_Settings.enabled = false;
        }
        return true;
    }

    if (m_Settings.autoAdvance) {
        AdvanceStage();
    } else {
        m_Settings.enabled = false;
    }

    return true;
}

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
