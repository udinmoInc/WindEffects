#include "Renderer/AtmosphereLUTGenerator.hpp"
#include "Renderer/RenderDiagnostics.hpp"
#include "Renderer/AtmosphereLUTInputs.hpp"
#include "Core/DiagnosticMacros.hpp"
#include "Core/LogCategory.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <stdexcept>
#if WE_HAS_GLM
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#endif

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

namespace {

constexpr VkFormat kLUTFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

float Clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

glm::vec3 SrgbToLinear(const glm::vec3& c) {
    glm::vec3 result;
    for (int i = 0; i < 3; ++i) {
        result[i] = c[i] <= 0.04045f ? c[i] / 12.92f : std::pow((c[i] + 0.055f) / 1.055f, 2.4f);
    }
    return result;
}

struct AtmosParams {
    glm::vec3 rayleigh;
    float mie;
    glm::vec3 ozone;
    float mieG;
    float planetR;
    float atmoR;
    float multiScatter;
    glm::vec3 sunColor;
    float sunIntensity;
    float sunAngularRadius;
    glm::vec3 sunDir;
};

float Density(float heightKm, float scale) {
    return std::exp(-std::max(heightKm, 0.0f) / std::max(scale, 1e-3f));
}

float OzoneDensity(float heightKm) {
    const float x = (heightKm - 25.0f) / 8.0f;
    return std::exp(-x * x);
}

glm::vec3 IntegrateInscatter(
    const glm::vec3& viewDir,
    const AtmosParams& p,
    int steps = 12) {
    const float start = 0.0f;
    const float end = p.atmoR - p.planetR;
    const float stepSize = end / static_cast<float>(steps);

    glm::vec3 sumR(0.0f);
    glm::vec3 sumM(0.0f);
    glm::vec3 opticalR(0.0f);
    glm::vec3 opticalM(0.0f);
    glm::vec3 opticalO(0.0f);

    const float cosTheta = glm::dot(glm::normalize(viewDir), p.sunDir);
    const float phaseR = 0.75f * (1.0f + cosTheta * cosTheta);
    const float g = p.mieG;
    const float g2 = g * g;
    const float phaseM = 1.5f * ((1.0f - g2) / std::pow(std::max(1.0f + g2 - 2.0f * g * cosTheta, 1e-4f), 1.5f)) * (1.0f + g2);

    for (int i = 0; i < steps; ++i) {
        const float t = start + (static_cast<float>(i) + 0.5f) * stepSize;
        const float heightKm = std::max(t, 0.0f);
        const float rd = Density(heightKm, 8.0f);
        const float md = Density(heightKm, 1.2f);
        const float od = OzoneDensity(heightKm);

        opticalR += p.rayleigh * rd * stepSize;
        opticalM += glm::vec3(p.mie) * md * stepSize;
        opticalO += p.ozone * od * stepSize;

        const glm::vec3 tau = opticalR + opticalM + opticalO;
        const glm::vec3 atten = glm::vec3(
            std::exp(-tau.x), std::exp(-tau.y), std::exp(-tau.z));

        sumR += atten * rd * stepSize;
        sumM += atten * md * stepSize;
    }

    const glm::vec3 multi = (glm::vec3(1.0f) - glm::vec3(
        std::exp(-(opticalR.x + opticalM.x) * 0.12f),
        std::exp(-(opticalR.y + opticalM.y) * 0.12f),
        std::exp(-(opticalR.z + opticalM.z) * 0.12f))) * p.rayleigh * p.multiScatter;

    const glm::vec3 sunRadiance = p.sunColor * p.sunIntensity;
    return sunRadiance * (sumR * phaseR * p.rayleigh + sumM * phaseM * p.mie + multi);
}

glm::vec3 SunDisk(const glm::vec3& viewDir, const AtmosParams& p) {
    const float cosAngle = glm::dot(glm::normalize(viewDir), p.sunDir);
    const float cosRadius = std::cos(p.sunAngularRadius);
    const float disk = Clamp01((cosAngle - cosRadius) / 0.00035f);
    const float glow = std::pow(Clamp01(glm::dot(viewDir, p.sunDir)), 256.0f) * p.sunIntensity * 0.15f;
    return p.sunColor * p.sunIntensity * (disk * 22.0f + glow);
}

AtmosParams BuildParams(const SceneEnvironmentUniform& env) {
    AtmosParams p{};
    p.rayleigh = env.atmosphereRayleigh;
    p.mie = env.mieScattering;
    p.ozone = env.ozoneAbsorption;
    p.mieG = env.mieAnisotropy;
    p.planetR = env.planetRadius;
    p.atmoR = env.planetRadius + env.atmosphereHeight;
    p.multiScatter = env.multiScatterStrength;
    p.sunColor = SrgbToLinear(glm::vec3(env.sunColor));
    p.sunIntensity = env.sunIntensity;
    p.sunAngularRadius = std::max(env.sunAngularRadius, 0.004675f);
    p.sunDir = glm::normalize(-glm::vec3(env.sunDirection));
    return p;
}

} // namespace

AtmosphereLUTGenerator::AtmosphereLUTGenerator(const std::shared_ptr<VulkanContext>& context)
    : m_Context(context) {
    CreateResources();
    CreateDescriptorResources();
}

AtmosphereLUTGenerator::~AtmosphereLUTGenerator() {
    DestroyDescriptorResources();
    DestroyResources();
}

void AtmosphereLUTGenerator::CreateLUTImage(
    uint32_t width,
    uint32_t height,
    VkImage& image,
    VkDeviceMemory& memory,
    VkImageView& view) {
    m_Context->CreateImage(
        width,
        height,
        kLUTFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        image,
        memory);
    view = m_Context->CreateImageView(image, kLUTFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

void AtmosphereLUTGenerator::CreateResources() {
    CreateLUTImage(m_Dimensions.transmittanceW, m_Dimensions.transmittanceH, m_Images.transmittance, m_Images.transmittanceMemory, m_Images.transmittanceView);
    CreateLUTImage(m_Dimensions.multiScatterSize, m_Dimensions.multiScatterSize, m_Images.multiScatter, m_Images.multiScatterMemory, m_Images.multiScatterView);
    CreateLUTImage(m_Dimensions.skyViewW, m_Dimensions.skyViewH, m_Images.skyView, m_Images.skyViewMemory, m_Images.skyViewView);
    CreateLUTImage(m_Dimensions.aerialSize, m_Dimensions.aerialSize, m_Images.aerial, m_Images.aerialMemory, m_Images.aerialView);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(m_Context->GetDevice(), &samplerInfo, nullptr, &m_Images.sampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create atmosphere LUT sampler.");
    }
}

void AtmosphereLUTGenerator::DestroyResources() {
    VkDevice device = m_Context->GetDevice();
    auto destroyImage = [&](VkImage& image, VkDeviceMemory& memory, VkImageView& view) {
        if (view != VK_NULL_HANDLE) vkDestroyImageView(device, view, nullptr);
        if (image != VK_NULL_HANDLE) vkDestroyImage(device, image, nullptr);
        if (memory != VK_NULL_HANDLE) vkFreeMemory(device, memory, nullptr);
        view = VK_NULL_HANDLE;
        image = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
    };

    destroyImage(m_Images.transmittance, m_Images.transmittanceMemory, m_Images.transmittanceView);
    destroyImage(m_Images.multiScatter, m_Images.multiScatterMemory, m_Images.multiScatterView);
    destroyImage(m_Images.skyView, m_Images.skyViewMemory, m_Images.skyViewView);
    destroyImage(m_Images.aerial, m_Images.aerialMemory, m_Images.aerialView);

    if (m_Images.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_Images.sampler, nullptr);
        m_Images.sampler = VK_NULL_HANDLE;
    }
}

void AtmosphereLUTGenerator::CreateDescriptorResources() {
    VkDevice device = m_Context->GetDevice();

    std::array<VkDescriptorSetLayoutBinding, 4> sampleBindings{
        VkDescriptorSetLayoutBinding{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        VkDescriptorSetLayoutBinding{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        VkDescriptorSetLayoutBinding{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        VkDescriptorSetLayoutBinding{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    VkDescriptorSetLayoutCreateInfo sampleLayoutInfo{};
    sampleLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    sampleLayoutInfo.bindingCount = static_cast<uint32_t>(sampleBindings.size());
    sampleLayoutInfo.pBindings = sampleBindings.data();
    if (vkCreateDescriptorSetLayout(device, &sampleLayoutInfo, nullptr, &m_SampleDescLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create atmosphere LUT sample descriptor layout.");
    }

    VkDescriptorSetAllocateInfo sampleAlloc{};
    sampleAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    sampleAlloc.descriptorPool = m_Context->GetDescriptorPool();
    sampleAlloc.descriptorSetCount = 1;
    sampleAlloc.pSetLayouts = &m_SampleDescLayout;
    const VkResult allocResult = vkAllocateDescriptorSets(device, &sampleAlloc, &m_SampleDescSet);
    if (allocResult != VK_SUCCESS) {
        m_SampleDescSet = VK_NULL_HANDLE;
        throw std::runtime_error("Failed to allocate atmosphere LUT sample descriptor set (VkResult=" + std::to_string(static_cast<int>(allocResult)) + ").");
    }
}

void AtmosphereLUTGenerator::DestroyDescriptorResources() {
    VkDevice device = m_Context->GetDevice();
    if (m_SampleDescLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_SampleDescLayout, nullptr);
    m_SampleDescLayout = VK_NULL_HANDLE;
    m_SampleDescSet = VK_NULL_HANDLE;
}

void AtmosphereLUTGenerator::TransitionLUTForUpload(VkImage image) {
    const VkImageLayout oldLayout = m_HasGenerated
        ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        : VK_IMAGE_LAYOUT_UNDEFINED;
    m_Context->TransitionImageLayout(image, kLUTFormat, oldLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
}

bool AtmosphereLUTGenerator::UploadLUT(VkImage image, uint32_t width, uint32_t height, const std::vector<float>& rgba, const char* lutName) {
    if (image == VK_NULL_HANDLE || width == 0 || height == 0) {
        m_LastError = std::string(lutName) + " LUT upload failed: image handle or dimensions are invalid.";
        return false;
    }

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    try {
        m_Context->CreateBuffer(
            rgba.size() * sizeof(float),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingMemory);
    } catch (const std::exception& ex) {
        m_LastError = std::string(lutName) + " LUT upload failed: staging buffer allocation error — " + ex.what();
        return false;
    }

    void* data = nullptr;
    vkMapMemory(m_Context->GetDevice(), stagingMemory, 0, rgba.size() * sizeof(float), 0, &data);
    std::memcpy(data, rgba.data(), rgba.size() * sizeof(float));
    vkUnmapMemory(m_Context->GetDevice(), stagingMemory);

    VkCommandBuffer cmd = m_Context->BeginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(cmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    m_Context->EndSingleTimeCommands(cmd);

    vkDestroyBuffer(m_Context->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Context->GetDevice(), stagingMemory, nullptr);

    m_Context->TransitionImageLayout(
        image,
        kLUTFormat,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    return true;
}

bool AtmosphereLUTGenerator::ValidateLUTData(const char* lutName, const std::vector<float>& rgba) {
    if (rgba.empty()) {
        m_LastError = std::string(lutName) + " LUT generation failed: produced zero texels.";
        return false;
    }

    bool hasFinite = false;
    for (size_t i = 0; i < rgba.size(); ++i) {
        if (!std::isfinite(rgba[i])) {
            m_LastError = std::string(lutName) + " LUT generation failed: non-finite value at component " + std::to_string(i) + ".";
            return false;
        }
        if (rgba[i] > 0.0f) {
            hasFinite = true;
        }
    }

    if (!hasFinite) {
        m_LastError = std::string(lutName) + " LUT generation failed: all texels are zero (no usable scattering data).";
        return false;
    }
    return true;
}

bool AtmosphereLUTGenerator::UpdateSampleDescriptors() {
    if (m_SampleDescSet == VK_NULL_HANDLE) {
        m_LastError = "Atmosphere LUT descriptor update failed: sample descriptor set is null.";
        return false;
    }
    if (m_Images.sampler == VK_NULL_HANDLE
        || m_Images.transmittanceView == VK_NULL_HANDLE
        || m_Images.multiScatterView == VK_NULL_HANDLE
        || m_Images.skyViewView == VK_NULL_HANDLE
        || m_Images.aerialView == VK_NULL_HANDLE) {
        m_LastError = "Atmosphere LUT descriptor update failed: one or more image views or sampler handles are null.";
        return false;
    }

    VkDevice device = m_Context->GetDevice();
    VkDescriptorImageInfo images[4] = {
        { m_Images.sampler, m_Images.transmittanceView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { m_Images.sampler, m_Images.multiScatterView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { m_Images.sampler, m_Images.skyViewView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { m_Images.sampler, m_Images.aerialView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
    };

    VkWriteDescriptorSet writes[4]{};
    for (uint32_t i = 0; i < 4; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = m_SampleDescSet;
        writes[i].dstBinding = i;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].descriptorCount = 1;
        writes[i].pImageInfo = &images[i];
    }
    vkUpdateDescriptorSets(device, 4, writes, 0, nullptr);
    return true;
}

bool AtmosphereLUTGenerator::ValidateResources(std::string* failureReason) const {
    auto fail = [&](const std::string& reason) {
        if (failureReason) {
            *failureReason = reason;
        }
        return false;
    };

    struct LUTResource {
        const char* name;
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
        uint32_t width;
        uint32_t height;
    };

    const LUTResource resources[] = {
        { "Transmittance", m_Images.transmittance, m_Images.transmittanceMemory, m_Images.transmittanceView, m_Dimensions.transmittanceW, m_Dimensions.transmittanceH },
        { "MultiScattering", m_Images.multiScatter, m_Images.multiScatterMemory, m_Images.multiScatterView, m_Dimensions.multiScatterSize, m_Dimensions.multiScatterSize },
        { "SkyView", m_Images.skyView, m_Images.skyViewMemory, m_Images.skyViewView, m_Dimensions.skyViewW, m_Dimensions.skyViewH },
        { "AerialPerspective", m_Images.aerial, m_Images.aerialMemory, m_Images.aerialView, m_Dimensions.aerialSize, m_Dimensions.aerialSize },
    };

    for (const LUTResource& lut : resources) {
        if (lut.image == VK_NULL_HANDLE || lut.memory == VK_NULL_HANDLE) {
            return fail(std::string(lut.name) + " LUT GPU allocation missing (image or device memory is null).");
        }
        if (lut.view == VK_NULL_HANDLE) {
            return fail(std::string(lut.name) + " LUT image view is null.");
        }
        if (lut.width == 0 || lut.height == 0) {
            return fail(std::string(lut.name) + " LUT has invalid dimensions.");
        }
    }

    if (m_Images.sampler == VK_NULL_HANDLE) {
        return fail("Atmosphere LUT sampler is null.");
    }
    if (m_SampleDescLayout == VK_NULL_HANDLE || m_SampleDescSet == VK_NULL_HANDLE) {
        return fail("Atmosphere LUT sample descriptor set or layout is null.");
    }
    if (!m_HasGenerated) {
        return fail("Atmosphere LUTs have not been generated yet.");
    }

    return true;
}

bool AtmosphereLUTGenerator::GenerateCPU(const SceneEnvironmentUniform& environment) {
    m_LastError.clear();
    m_Ready = false;

    const AtmosParams params = BuildParams(environment);

    std::vector<float> transmittance(m_Dimensions.transmittanceW * m_Dimensions.transmittanceH * 4, 0.0f);
    for (uint32_t y = 0; y < m_Dimensions.transmittanceH; ++y) {
        for (uint32_t x = 0; x < m_Dimensions.transmittanceW; ++x) {
            const float hNorm = (static_cast<float>(x) + 0.5f) / static_cast<float>(m_Dimensions.transmittanceW);
            const float muNorm = (static_cast<float>(y) + 0.5f) / static_cast<float>(m_Dimensions.transmittanceH);
            const float heightKm = hNorm * (params.atmoR - params.planetR);
            const float density = Density(heightKm, 8.0f);
            const float t = std::exp(-density * (1.0f - muNorm) * 6.0f);
            const size_t idx = (y * m_Dimensions.transmittanceW + x) * 4;
            transmittance[idx + 0] = t;
            transmittance[idx + 1] = t;
            transmittance[idx + 2] = t;
            transmittance[idx + 3] = 1.0f;
        }
    }

    std::vector<float> multiScatter(m_Dimensions.multiScatterSize * m_Dimensions.multiScatterSize * 4, 0.0f);
    for (uint32_t y = 0; y < m_Dimensions.multiScatterSize; ++y) {
        for (uint32_t x = 0; x < m_Dimensions.multiScatterSize; ++x) {
            const float cosSun = (static_cast<float>(x) + 0.5f) / static_cast<float>(m_Dimensions.multiScatterSize) * 2.0f - 1.0f;
            const glm::vec3 ms = params.rayleigh * (0.5f + cosSun * 0.5f) * params.multiScatter * 0.4f;
            const size_t idx = (y * m_Dimensions.multiScatterSize + x) * 4;
            multiScatter[idx + 0] = ms.x;
            multiScatter[idx + 1] = ms.y;
            multiScatter[idx + 2] = ms.z;
            multiScatter[idx + 3] = 1.0f;
        }
    }

    std::vector<float> skyView(m_Dimensions.skyViewW * m_Dimensions.skyViewH * 4, 0.0f);
    for (uint32_t y = 0; y < m_Dimensions.skyViewH; ++y) {
        for (uint32_t x = 0; x < m_Dimensions.skyViewW; ++x) {
            const float azimuth = (static_cast<float>(x) + 0.5f) / static_cast<float>(m_Dimensions.skyViewW);
            const float viewZenith = (static_cast<float>(y) + 0.5f) / static_cast<float>(m_Dimensions.skyViewH);
            const float theta = viewZenith * 3.14159265f;
            const float phi = azimuth * 2.0f * 3.14159265f;
            const glm::vec3 viewDir(
                std::sin(theta) * std::cos(phi),
                std::cos(theta),
                std::sin(theta) * std::sin(phi));

            glm::vec3 sky = IntegrateInscatter(viewDir, params);
            sky += SunDisk(viewDir, params);
            sky = glm::max(sky, glm::vec3(0.0f));

            const size_t idx = (y * m_Dimensions.skyViewW + x) * 4;
            skyView[idx + 0] = sky.x;
            skyView[idx + 1] = sky.y;
            skyView[idx + 2] = sky.z;
            skyView[idx + 3] = 1.0f;
        }
    }

    std::vector<float> aerial(m_Dimensions.aerialSize * m_Dimensions.aerialSize * 4, 0.0f);
    for (uint32_t y = 0; y < m_Dimensions.aerialSize; ++y) {
        for (uint32_t x = 0; x < m_Dimensions.aerialSize; ++x) {
            const float distKm = (static_cast<float>(x) + 0.5f) / static_cast<float>(m_Dimensions.aerialSize) * 64.0f;
            const glm::vec3 viewDir = glm::normalize(glm::vec3(0.3f, 0.15f, 1.0f));
            glm::vec3 inscatter = IntegrateInscatter(viewDir, params);
            inscatter *= (1.0f - std::exp(-distKm * 0.06f));
            const size_t idx = (y * m_Dimensions.aerialSize + x) * 4;
            aerial[idx + 0] = inscatter.x;
            aerial[idx + 1] = inscatter.y;
            aerial[idx + 2] = inscatter.z;
            aerial[idx + 3] = 1.0f;
        }
    }

    struct LUTUpload {
        const char* name;
        VkImage image;
        uint32_t width;
        uint32_t height;
        std::vector<float>& data;
    };

    LUTUpload uploads[] = {
        { "Transmittance", m_Images.transmittance, m_Dimensions.transmittanceW, m_Dimensions.transmittanceH, transmittance },
        { "MultiScattering", m_Images.multiScatter, m_Dimensions.multiScatterSize, m_Dimensions.multiScatterSize, multiScatter },
        { "SkyView", m_Images.skyView, m_Dimensions.skyViewW, m_Dimensions.skyViewH, skyView },
        { "AerialPerspective", m_Images.aerial, m_Dimensions.aerialSize, m_Dimensions.aerialSize, aerial },
    };

    for (LUTUpload& upload : uploads) {
        if (!ValidateLUTData(upload.name, upload.data)) {
            RenderDiagnostics::Get().Emit(
                DiagnosticSeverity::Error,
                we::runtime::core::LogCategory::Environment.data(),
                m_LastError,
                "Check atmosphere environment parameters (sun intensity, scattering coefficients, planet radius).");
            return false;
        }
        TransitionLUTForUpload(upload.image);
        if (!UploadLUT(upload.image, upload.width, upload.height, upload.data, upload.name)) {
            RenderDiagnostics::Get().Emit(
                DiagnosticSeverity::Error,
                we::runtime::core::LogCategory::Environment.data(),
                m_LastError,
                "Verify GPU memory availability and Vulkan transfer paths.");
            return false;
        }
    }

    if (!UpdateSampleDescriptors()) {
        RenderDiagnostics::Get().Emit(
            DiagnosticSeverity::Error,
            we::runtime::core::LogCategory::Resource.data(),
            m_LastError,
            "Ensure descriptor pool has free combined image sampler slots.");
        return false;
    }

    m_HasGenerated = true;

    std::string validationError;
    if (!ValidateResources(&validationError)) {
        m_HasGenerated = false;
        m_LastError = validationError;
        RenderDiagnostics::Get().Emit(
            DiagnosticSeverity::Error,
            we::runtime::core::LogCategory::Environment.data(),
            m_LastError,
            "Regenerate atmosphere LUTs after fixing resource creation failures.");
        return false;
    }

    m_Ready = true;
    return true;
}

bool AtmosphereLUTGenerator::EnsureGenerated(const SceneEnvironmentUniform& environment) {
    if (!m_Dirty && m_Ready && m_HasGenerated && !AtmosphereLUTInputsChanged(m_LastGeneratedEnvironment, environment)) {
        return true;
    }

    m_Dirty = false;
    if (!GenerateCPU(environment)) {
        m_Ready = false;
        return false;
    }

    m_LastGeneratedEnvironment = environment;
    WE_LOG_INFO(we::runtime::core::LogCategory::Environment.data(),
        "Atmosphere LUTs generated: Transmittance " + std::to_string(m_Dimensions.transmittanceW) + "x" + std::to_string(m_Dimensions.transmittanceH)
        + ", MultiScattering " + std::to_string(m_Dimensions.multiScatterSize) + "x" + std::to_string(m_Dimensions.multiScatterSize)
        + ", SkyView " + std::to_string(m_Dimensions.skyViewW) + "x" + std::to_string(m_Dimensions.skyViewH)
        + ", AerialPerspective " + std::to_string(m_Dimensions.aerialSize) + "x" + std::to_string(m_Dimensions.aerialSize) + ".");
    return true;
}

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
