#include "Renderer/AtmosphereLUTGenerator.hpp"
#include "Renderer/SceneRenderer.hpp"
#include "Core/DiagnosticMacros.hpp"
#include "Core/LogCategory.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#if WE_HAS_GLM
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#endif

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

namespace {

constexpr uint32_t kTransmittanceW = 256;
constexpr uint32_t kTransmittanceH = 64;
constexpr uint32_t kMultiScatterSize = 32;
constexpr uint32_t kSkyViewW = 192;
constexpr uint32_t kSkyViewH = 96;
constexpr uint32_t kAerialSize = 32;

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
    return p.sunColor * p.sunIntensity * (disk * 28.0f + glow);
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
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        image,
        memory);
    view = m_Context->CreateImageView(image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
}

void AtmosphereLUTGenerator::CreateResources() {
    CreateLUTImage(kTransmittanceW, kTransmittanceH, m_Images.transmittance, m_Images.transmittanceMemory, m_Images.transmittanceView);
    CreateLUTImage(kMultiScatterSize, kMultiScatterSize, m_Images.multiScatter, m_Images.multiScatterMemory, m_Images.multiScatterView);
    CreateLUTImage(kSkyViewW, kSkyViewH, m_Images.skyView, m_Images.skyViewMemory, m_Images.skyViewView);
    CreateLUTImage(kAerialSize, kAerialSize, m_Images.aerial, m_Images.aerialMemory, m_Images.aerialView);

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
    vkAllocateDescriptorSets(device, &sampleAlloc, &m_SampleDescSet);
}

void AtmosphereLUTGenerator::DestroyDescriptorResources() {
    VkDevice device = m_Context->GetDevice();
    if (m_SampleDescLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_SampleDescLayout, nullptr);
    m_SampleDescLayout = VK_NULL_HANDLE;
}

void AtmosphereLUTGenerator::UploadLUT(VkImage image, uint32_t width, uint32_t height, const std::vector<float>& rgba) {
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    m_Context->CreateBuffer(
        rgba.size() * sizeof(float),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

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
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void AtmosphereLUTGenerator::GenerateCPU(const SceneEnvironmentUniform& environment) {
    const AtmosParams params = BuildParams(environment);

    std::vector<float> transmittance(kTransmittanceW * kTransmittanceH * 4, 0.0f);
    for (uint32_t y = 0; y < kTransmittanceH; ++y) {
        for (uint32_t x = 0; x < kTransmittanceW; ++x) {
            const float hNorm = (static_cast<float>(x) + 0.5f) / static_cast<float>(kTransmittanceW);
            const float muNorm = (static_cast<float>(y) + 0.5f) / static_cast<float>(kTransmittanceH);
            const float heightKm = hNorm * (params.atmoR - params.planetR);
            const float density = Density(heightKm, 8.0f);
            const float t = std::exp(-density * (1.0f - muNorm) * 6.0f);
            const size_t idx = (y * kTransmittanceW + x) * 4;
            transmittance[idx + 0] = t;
            transmittance[idx + 1] = t;
            transmittance[idx + 2] = t;
            transmittance[idx + 3] = 1.0f;
        }
    }

    std::vector<float> multiScatter(kMultiScatterSize * kMultiScatterSize * 4, 0.0f);
    for (uint32_t y = 0; y < kMultiScatterSize; ++y) {
        for (uint32_t x = 0; x < kMultiScatterSize; ++x) {
            const float cosSun = (static_cast<float>(x) + 0.5f) / static_cast<float>(kMultiScatterSize) * 2.0f - 1.0f;
            const glm::vec3 ms = params.rayleigh * (0.5f + cosSun * 0.5f) * params.multiScatter * 0.4f;
            const size_t idx = (y * kMultiScatterSize + x) * 4;
            multiScatter[idx + 0] = ms.x;
            multiScatter[idx + 1] = ms.y;
            multiScatter[idx + 2] = ms.z;
            multiScatter[idx + 3] = 1.0f;
        }
    }

    std::vector<float> skyView(kSkyViewW * kSkyViewH * 4, 0.0f);
    for (uint32_t y = 0; y < kSkyViewH; ++y) {
        for (uint32_t x = 0; x < kSkyViewW; ++x) {
            const float azimuth = (static_cast<float>(x) + 0.5f) / static_cast<float>(kSkyViewW);
            const float viewZenith = (static_cast<float>(y) + 0.5f) / static_cast<float>(kSkyViewH);
            const float theta = viewZenith * 3.14159265f;
            const float phi = azimuth * 2.0f * 3.14159265f;
            const glm::vec3 viewDir(
                std::sin(theta) * std::cos(phi),
                std::cos(theta),
                std::sin(theta) * std::sin(phi));

            glm::vec3 sky = IntegrateInscatter(viewDir, params);
            sky += SunDisk(viewDir, params);
            sky = glm::max(sky, glm::vec3(0.0f));

            const size_t idx = (y * kSkyViewW + x) * 4;
            skyView[idx + 0] = sky.x;
            skyView[idx + 1] = sky.y;
            skyView[idx + 2] = sky.z;
            skyView[idx + 3] = 1.0f;
        }
    }

    std::vector<float> aerial(kAerialSize * kAerialSize * 4, 0.0f);
    for (uint32_t y = 0; y < kAerialSize; ++y) {
        for (uint32_t x = 0; x < kAerialSize; ++x) {
            const float distKm = (static_cast<float>(x) + 0.5f) / static_cast<float>(kAerialSize) * 64.0f;
            const glm::vec3 viewDir = glm::normalize(glm::vec3(0.3f, 0.15f, 1.0f));
            glm::vec3 inscatter = IntegrateInscatter(viewDir, params);
            inscatter *= (1.0f - std::exp(-distKm * 0.06f));
            const size_t idx = (y * kAerialSize + x) * 4;
            aerial[idx + 0] = inscatter.x;
            aerial[idx + 1] = inscatter.y;
            aerial[idx + 2] = inscatter.z;
            aerial[idx + 3] = 1.0f;
        }
    }

    m_Context->TransitionImageLayout(m_Images.transmittance, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    m_Context->TransitionImageLayout(m_Images.multiScatter, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    m_Context->TransitionImageLayout(m_Images.skyView, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    m_Context->TransitionImageLayout(m_Images.aerial, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    UploadLUT(m_Images.transmittance, kTransmittanceW, kTransmittanceH, transmittance);
    UploadLUT(m_Images.multiScatter, kMultiScatterSize, kMultiScatterSize, multiScatter);
    UploadLUT(m_Images.skyView, kSkyViewW, kSkyViewH, skyView);
    UploadLUT(m_Images.aerial, kAerialSize, kAerialSize, aerial);

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
}

void AtmosphereLUTGenerator::EnsureGenerated(const SceneEnvironmentUniform& environment) {
    if (!m_Dirty) {
        return;
    }
    m_Dirty = false;
    GenerateCPU(environment);
    WE_LOG_INFO(we::runtime::core::LogCategory::Environment.data(), "Regenerated procedural sky LUTs.");
}

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
