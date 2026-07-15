#include "Rendering/UiImmediateRenderer.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "RHI/Desc.h"
#include "RHI/ShaderBytecode.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace WindEffects::Editor::UI {
namespace {

struct TextPushConstants {
    float uScale[2];
    float uTranslate[2];
    float uAtlasSize[2];
    float uPixelRange;
    float padding;
};

static_assert(sizeof(TextPushConstants) == 32, "Text push constants must match TextMSDF.hlsl");
static_assert(sizeof(we::rhi::UIVertex) == sizeof(float) * 16, "UIVertex stride");

void FillUiTransform(uint32_t width, uint32_t height, float out[4]) {
    out[0] = 2.0f / static_cast<float>(std::max(width, 1u));
    out[1] = 2.0f / static_cast<float>(std::max(height, 1u));
    out[2] = -1.0f;
    out[3] = -1.0f;
}

void FillTextPush(
    uint32_t width,
    uint32_t height,
    uint32_t atlasWidth,
    uint32_t atlasHeight,
    float msdfPixelRange,
    TextPushConstants& out)
{
    float transform[4];
    FillUiTransform(width, height, transform);
    out.uScale[0] = transform[0];
    out.uScale[1] = transform[1];
    out.uTranslate[0] = transform[2];
    out.uTranslate[1] = transform[3];
    out.uAtlasSize[0] = static_cast<float>(std::max(atlasWidth, 1u));
    out.uAtlasSize[1] = static_cast<float>(std::max(atlasHeight, 1u));
    out.uPixelRange = std::max(msdfPixelRange, 1.0f);
    out.padding = 0.0f;
}

void FillVertexLayout(we::rhi::GraphicsPipelineDesc& pso) {
    we::rhi::VertexBindingDesc binding{};
    binding.binding = 0;
    binding.stride = sizeof(we::rhi::UIVertex);
    binding.perInstance = false;
    pso.vertexBindings = {binding};

    pso.vertexAttributes = {
        {0, 0, we::rhi::Format::R32G32_SFLOAT, static_cast<uint32_t>(offsetof(we::rhi::UIVertex, position))},
        {1, 0, we::rhi::Format::R32G32_SFLOAT, static_cast<uint32_t>(offsetof(we::rhi::UIVertex, uv))},
        {2, 0, we::rhi::Format::R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(we::rhi::UIVertex, color))},
        {3, 0, we::rhi::Format::R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(we::rhi::UIVertex, sdfRect))},
        {4, 0, we::rhi::Format::R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(we::rhi::UIVertex, sdfParams))},
    };
}

void FillUiBlend(we::rhi::BlendStateDesc& blend) {
    blend.enable = true;
    blend.srcColor = we::rhi::BlendFactor::SrcAlpha;
    blend.dstColor = we::rhi::BlendFactor::OneMinusSrcAlpha;
    blend.colorOp = we::rhi::BlendOp::Add;
    blend.srcAlpha = we::rhi::BlendFactor::One;
    blend.dstAlpha = we::rhi::BlendFactor::Zero;
    blend.alphaOp = we::rhi::BlendOp::Add;
}

} // namespace

UiImmediateRenderer::~UiImmediateRenderer() {
    Shutdown();
}

bool UiImmediateRenderer::Init(
    we::rhi::IRHIDevice* device,
    we::rhi::Format swapchainFormat,
    uint32_t framesInFlight)
{
    Shutdown();
    m_Device = device;
    if (!m_Device) {
        return false;
    }
    m_SwapchainFormat = swapchainFormat != we::rhi::Format::Unknown
        ? swapchainFormat
        : we::rhi::Format::B8G8R8A8_UNORM;
    m_MaxFramesInFlight = framesInFlight ? framesInFlight : 2;

    if (!LoadShaders()) {
        WE_LOG_ERROR(we::LogCategory::Startup, "UiImmediateRenderer: failed to load UI/TextMSDF shaders.");
        Shutdown();
        return false;
    }

    we::rhi::DescriptorSetLayoutDesc layoutDesc{};
    layoutDesc.debugName = "UiImmediate.TextureLayout";
    layoutDesc.bindings.push_back({
        0,
        we::rhi::DescriptorType::CombinedImageSampler,
        1,
        we::rhi::ShaderStageFlags::Fragment});
    auto layout = m_Device->CreateDescriptorSetLayout(layoutDesc);
    if (!layout) {
        Shutdown();
        return false;
    }
    m_TextureLayout = *layout;

    we::rhi::PipelineLayoutDesc uiLayoutDesc{};
    uiLayoutDesc.debugName = "UiImmediate.UILayout";
    uiLayoutDesc.setLayouts = {m_TextureLayout};
    uiLayoutDesc.pushConstantSize = sizeof(float) * 4;
    uiLayoutDesc.pushConstantStages = we::rhi::ShaderStageFlags::Vertex;
    auto uiLayout = m_Device->CreatePipelineLayout(uiLayoutDesc);
    if (!uiLayout) {
        Shutdown();
        return false;
    }
    m_UiLayout = *uiLayout;

    we::rhi::PipelineLayoutDesc textLayoutDesc{};
    textLayoutDesc.debugName = "UiImmediate.TextLayout";
    textLayoutDesc.setLayouts = {m_TextureLayout};
    textLayoutDesc.pushConstantSize = sizeof(TextPushConstants);
    textLayoutDesc.pushConstantStages =
        we::rhi::ShaderStageFlags::Vertex | we::rhi::ShaderStageFlags::Fragment;
    auto textLayout = m_Device->CreatePipelineLayout(textLayoutDesc);
    if (!textLayout) {
        Shutdown();
        return false;
    }
    m_TextLayout = *textLayout;

    we::rhi::DescriptorPoolDesc poolDesc{};
    poolDesc.maxSets = 2048;
    poolDesc.debugName = "UiImmediate.Pool";
    poolDesc.poolSizes.push_back({we::rhi::DescriptorType::CombinedImageSampler, 4096});
    auto pool = m_Device->CreateDescriptorPool(poolDesc);
    if (!pool) {
        Shutdown();
        return false;
    }
    m_Pool = *pool;

    if (!CreatePipelines()) {
        Shutdown();
        return false;
    }
    if (!CreateDummyTexture()) {
        Shutdown();
        return false;
    }

    m_FrameGeometry.resize(m_MaxFramesInFlight);
    m_Ready = true;
    WE_LOG_INFO(we::LogCategory::Startup, "UiImmediateRenderer ready (IRHI).");
    return true;
}

void UiImmediateRenderer::Shutdown() {
    std::lock_guard lock(m_Mutex);
    if (!m_Device) {
        m_Ready = false;
        m_Cmd = nullptr;
        m_InRenderPass = false;
        return;
    }

    for (auto& [key, uploaded] : m_Uploaded) {
        (void)key;
        DestroyUploaded(uploaded);
    }
    m_Uploaded.clear();

    if (m_DummySet != we::rhi::RHIDescriptorSetHandle::Invalid) {
        UploadedTexture dummy{};
        dummy.texture = m_DummyTexture;
        dummy.view = m_DummyView;
        dummy.sampler = m_DummySampler;
        dummy.set = m_DummySet;
        DestroyUploaded(dummy);
        m_DummyTexture = we::rhi::RHITextureHandle::Invalid;
        m_DummyView = we::rhi::RHITextureViewHandle::Invalid;
        m_DummySampler = we::rhi::RHISamplerHandle::Invalid;
        m_DummySet = we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    for (auto& frame : m_FrameGeometry) {
        if (frame.vertexBuffer != we::rhi::RHIBufferHandle::Invalid) {
            (void)m_Device->DestroyBuffer(frame.vertexBuffer);
            frame.vertexBuffer = we::rhi::RHIBufferHandle::Invalid;
        }
        if (frame.indexBuffer != we::rhi::RHIBufferHandle::Invalid) {
            (void)m_Device->DestroyBuffer(frame.indexBuffer);
            frame.indexBuffer = we::rhi::RHIBufferHandle::Invalid;
        }
        frame.vertexCapacity = 0;
        frame.indexCapacity = 0;
    }
    m_FrameGeometry.clear();

    if (m_UiPipeline != we::rhi::RHIGraphicsPipelineHandle::Invalid) {
        (void)m_Device->DestroyGraphicsPipeline(m_UiPipeline);
        m_UiPipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;
    }
    if (m_TextPipeline != we::rhi::RHIGraphicsPipelineHandle::Invalid) {
        (void)m_Device->DestroyGraphicsPipeline(m_TextPipeline);
        m_TextPipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;
    }
    if (m_UiLayout != we::rhi::RHIPipelineLayoutHandle::Invalid) {
        (void)m_Device->DestroyPipelineLayout(m_UiLayout);
        m_UiLayout = we::rhi::RHIPipelineLayoutHandle::Invalid;
    }
    if (m_TextLayout != we::rhi::RHIPipelineLayoutHandle::Invalid) {
        (void)m_Device->DestroyPipelineLayout(m_TextLayout);
        m_TextLayout = we::rhi::RHIPipelineLayoutHandle::Invalid;
    }
    if (m_Pool != we::rhi::RHIDescriptorPoolHandle::Invalid) {
        (void)m_Device->DestroyDescriptorPool(m_Pool);
        m_Pool = we::rhi::RHIDescriptorPoolHandle::Invalid;
    }
    if (m_TextureLayout != we::rhi::RHIDescriptorSetLayoutHandle::Invalid) {
        (void)m_Device->DestroyDescriptorSetLayout(m_TextureLayout);
        m_TextureLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    }
    if (m_UiVs != we::rhi::RHIShaderHandle::Invalid) {
        (void)m_Device->DestroyShader(m_UiVs);
        m_UiVs = we::rhi::RHIShaderHandle::Invalid;
    }
    if (m_UiPs != we::rhi::RHIShaderHandle::Invalid) {
        (void)m_Device->DestroyShader(m_UiPs);
        m_UiPs = we::rhi::RHIShaderHandle::Invalid;
    }
    if (m_TextVs != we::rhi::RHIShaderHandle::Invalid) {
        (void)m_Device->DestroyShader(m_TextVs);
        m_TextVs = we::rhi::RHIShaderHandle::Invalid;
    }
    if (m_TextPs != we::rhi::RHIShaderHandle::Invalid) {
        (void)m_Device->DestroyShader(m_TextPs);
        m_TextPs = we::rhi::RHIShaderHandle::Invalid;
    }

    m_Device = nullptr;
    m_Cmd = nullptr;
    m_BoundSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    m_InRenderPass = false;
    m_Ready = false;
}

bool UiImmediateRenderer::LoadShaders() {
    const auto format = we::rhi::ShaderBytecodeLoader::ResolveFormat(m_Device);
    m_UiVsBytes = we::rhi::ShaderBytecodeLoader::Load("UI", "VS", format);
    m_UiPsBytes = we::rhi::ShaderBytecodeLoader::Load("UI", "PS", format);
    m_TextVsBytes = we::rhi::ShaderBytecodeLoader::Load("TextMSDF", "VS", format);
    m_TextPsBytes = we::rhi::ShaderBytecodeLoader::Load("TextMSDF", "PS", format);
    if (m_UiVsBytes.empty() || m_UiPsBytes.empty() || m_TextVsBytes.empty() || m_TextPsBytes.empty()) {
        return false;
    }

    auto create = [&](we::rhi::ShaderStage stage, const std::vector<uint8_t>& bytes, const char* name)
        -> we::rhi::RHIShaderHandle {
        we::rhi::ShaderDesc desc{};
        desc.stage = stage;
        desc.format = format;
        desc.bytecode = bytes;
        desc.entryPoint = (stage == we::rhi::ShaderStage::Fragment) ? "PSMain" : "VSMain";
        desc.debugName = name;
        auto handle = m_Device->CreateShader(desc);
        return handle ? *handle : we::rhi::RHIShaderHandle::Invalid;
    };

    m_UiVs = create(we::rhi::ShaderStage::Vertex, m_UiVsBytes, "UI.VS");
    m_UiPs = create(we::rhi::ShaderStage::Fragment, m_UiPsBytes, "UI.PS");
    m_TextVs = create(we::rhi::ShaderStage::Vertex, m_TextVsBytes, "TextMSDF.VS");
    m_TextPs = create(we::rhi::ShaderStage::Fragment, m_TextPsBytes, "TextMSDF.PS");
    return m_UiVs != we::rhi::RHIShaderHandle::Invalid
        && m_UiPs != we::rhi::RHIShaderHandle::Invalid
        && m_TextVs != we::rhi::RHIShaderHandle::Invalid
        && m_TextPs != we::rhi::RHIShaderHandle::Invalid;
}

bool UiImmediateRenderer::CreatePipelines() {
    we::rhi::GraphicsPipelineDesc uiPso{};
    uiPso.vertexShader = m_UiVs;
    uiPso.fragmentShader = m_UiPs;
    uiPso.layout = m_UiLayout;
    uiPso.topology = we::rhi::PrimitiveTopology::TriangleList;
    uiPso.cullMode = we::rhi::CullMode::None;
    uiPso.depthTest = false;
    uiPso.depthWrite = false;
    uiPso.depthAttachment = false;
    uiPso.colorFormat = m_SwapchainFormat;
    FillVertexLayout(uiPso);
    FillUiBlend(uiPso.blend);
    uiPso.debugName = "UiImmediate.UI";
    auto uiPipe = m_Device->CreateGraphicsPipeline(uiPso);
    if (!uiPipe) {
        WE_LOG_ERROR(we::LogCategory::Startup, "UiImmediateRenderer: UI CreateGraphicsPipeline failed.");
        return false;
    }
    m_UiPipeline = *uiPipe;

    we::rhi::GraphicsPipelineDesc textPso = uiPso;
    textPso.vertexShader = m_TextVs;
    textPso.fragmentShader = m_TextPs;
    textPso.layout = m_TextLayout;
    textPso.debugName = "UiImmediate.TextMSDF";
    auto textPipe = m_Device->CreateGraphicsPipeline(textPso);
    if (!textPipe) {
        WE_LOG_ERROR(we::LogCategory::Startup, "UiImmediateRenderer: Text CreateGraphicsPipeline failed.");
        return false;
    }
    m_TextPipeline = *textPipe;
    return true;
}

bool UiImmediateRenderer::CreateDummyTexture() {
    const std::array<uint8_t, 4> pixel = {255, 255, 255, 255};
    auto set = UploadRgbaTexture(1, 1, pixel, false);
    if (set == we::rhi::RHIDescriptorSetHandle::Invalid) {
        return false;
    }
    auto it = m_Uploaded.find(static_cast<uint64_t>(set));
    if (it == m_Uploaded.end()) {
        return false;
    }
    m_DummyTexture = it->second.texture;
    m_DummyView = it->second.view;
    m_DummySampler = it->second.sampler;
    m_DummySet = it->second.set;
    // Own dummy separately from the uploaded destroy map.
    m_Uploaded.erase(it);
    return true;
}

void UiImmediateRenderer::WriteTextureSet(
    we::rhi::RHIDescriptorSetHandle set,
    we::rhi::RHITextureViewHandle view,
    we::rhi::RHISamplerHandle sampler)
{
    we::rhi::DescriptorImageInfo imageInfo{};
    imageInfo.sampler = sampler;
    imageInfo.view = view;
    imageInfo.imageLayout = we::rhi::ResourceState::ShaderResource;

    we::rhi::WriteDescriptorSet write{};
    write.set = set;
    write.binding = 0;
    write.type = we::rhi::DescriptorType::CombinedImageSampler;
    write.count = 1;
    write.imageInfos = &imageInfo;
    m_Device->UpdateDescriptorSets(std::span<const we::rhi::WriteDescriptorSet>(&write, 1));
}

we::rhi::RHIDescriptorSetHandle UiImmediateRenderer::AllocateTextureSet(
    we::rhi::RHITextureViewHandle view,
    we::rhi::RHISamplerHandle sampler)
{
    if (!m_Device || view == we::rhi::RHITextureViewHandle::Invalid
        || sampler == we::rhi::RHISamplerHandle::Invalid
        || m_Pool == we::rhi::RHIDescriptorPoolHandle::Invalid) {
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    we::rhi::DescriptorSetAllocateDesc alloc{};
    alloc.pool = m_Pool;
    alloc.layout = m_TextureLayout;
    alloc.debugName = "UiImmediate.TextureSet";
    auto set = m_Device->AllocateDescriptorSet(alloc);
    if (!set) {
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }
    WriteTextureSet(*set, view, sampler);
    return *set;
}

void UiImmediateRenderer::DestroyUploaded(UploadedTexture& uploaded) {
    if (!m_Device) {
        return;
    }
    if (uploaded.sampler != we::rhi::RHISamplerHandle::Invalid) {
        (void)m_Device->DestroySampler(uploaded.sampler);
        uploaded.sampler = we::rhi::RHISamplerHandle::Invalid;
    }
    if (uploaded.view != we::rhi::RHITextureViewHandle::Invalid) {
        (void)m_Device->DestroyTextureView(uploaded.view);
        uploaded.view = we::rhi::RHITextureViewHandle::Invalid;
    }
    if (uploaded.texture != we::rhi::RHITextureHandle::Invalid) {
        (void)m_Device->DestroyTexture(uploaded.texture);
        uploaded.texture = we::rhi::RHITextureHandle::Invalid;
    }
    uploaded.set = we::rhi::RHIDescriptorSetHandle::Invalid;
}

we::rhi::RHIDescriptorSetHandle UiImmediateRenderer::RegisterTexture(
    we::rhi::RHITextureViewHandle view,
    we::rhi::RHISamplerHandle sampler)
{
    std::lock_guard lock(m_Mutex);
    return AllocateTextureSet(view, sampler);
}

void UiImmediateRenderer::UpdateTexture(
    we::rhi::RHIDescriptorSetHandle set,
    we::rhi::RHITextureViewHandle view,
    we::rhi::RHISamplerHandle sampler)
{
    std::lock_guard lock(m_Mutex);
    if (set == we::rhi::RHIDescriptorSetHandle::Invalid
        || view == we::rhi::RHITextureViewHandle::Invalid
        || sampler == we::rhi::RHISamplerHandle::Invalid
        || !m_Device) {
        return;
    }
    WriteTextureSet(set, view, sampler);
}

void UiImmediateRenderer::UnregisterTexture(we::rhi::RHIDescriptorSetHandle set) {
    std::lock_guard lock(m_Mutex);
    if (set == we::rhi::RHIDescriptorSetHandle::Invalid || set == m_DummySet) {
        return;
    }
    auto it = m_Uploaded.find(static_cast<uint64_t>(set));
    if (it != m_Uploaded.end()) {
        DestroyUploaded(it->second);
        m_Uploaded.erase(it);
    }
}

we::rhi::RHIDescriptorSetHandle UiImmediateRenderer::UploadRgbaTexture(
    uint32_t width,
    uint32_t height,
    std::span<const uint8_t> rgba,
    bool linearFilter)
{
    std::lock_guard lock(m_Mutex);
    if (!m_Device || width == 0 || height == 0
        || rgba.size() < static_cast<size_t>(width) * height * 4) {
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    we::rhi::TextureDesc texDesc{};
    texDesc.extent = {width, height, 1};
    texDesc.format = we::rhi::Format::R8G8B8A8_UNORM;
    texDesc.usage = we::rhi::TextureUsage::Sampled | we::rhi::TextureUsage::TransferDst;
    texDesc.debugName = "UiImmediate.Atlas";
    auto tex = m_Device->CreateTexture(texDesc);
    if (!tex) {
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    we::rhi::TextureUpdateDesc update{};
    update.extent = {width, height, 1};
    update.data = rgba;
    if (!m_Device->UpdateTexture(*tex, update)) {
        (void)m_Device->DestroyTexture(*tex);
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    we::rhi::TextureViewDesc viewDesc{};
    viewDesc.texture = *tex;
    auto view = m_Device->CreateTextureView(viewDesc);
    if (!view) {
        (void)m_Device->DestroyTexture(*tex);
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    we::rhi::SamplerDesc samplerDesc{};
    samplerDesc.magFilter = linearFilter ? we::rhi::Filter::Linear : we::rhi::Filter::Nearest;
    samplerDesc.minFilter = samplerDesc.magFilter;
    samplerDesc.mipFilter = we::rhi::Filter::Nearest;
    samplerDesc.addressU = we::rhi::AddressMode::ClampToEdge;
    samplerDesc.addressV = we::rhi::AddressMode::ClampToEdge;
    samplerDesc.addressW = we::rhi::AddressMode::ClampToEdge;
    samplerDesc.anisotropy = false;
    auto sampler = m_Device->CreateSampler(samplerDesc);
    if (!sampler) {
        (void)m_Device->DestroyTextureView(*view);
        (void)m_Device->DestroyTexture(*tex);
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    auto set = AllocateTextureSet(*view, *sampler);
    if (set == we::rhi::RHIDescriptorSetHandle::Invalid) {
        (void)m_Device->DestroySampler(*sampler);
        (void)m_Device->DestroyTextureView(*view);
        (void)m_Device->DestroyTexture(*tex);
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    UploadedTexture uploaded{};
    uploaded.texture = *tex;
    uploaded.view = *view;
    uploaded.sampler = *sampler;
    uploaded.set = set;
    m_Uploaded.emplace(static_cast<uint64_t>(set), uploaded);
    return set;
}

bool UiImmediateRenderer::EnsureBuffer(
    we::rhi::RHIBufferHandle& handle,
    uint64_t& capacity,
    uint64_t required,
    we::rhi::BufferUsage usage,
    const char* debugName)
{
    if (required <= capacity && handle != we::rhi::RHIBufferHandle::Invalid) {
        return true;
    }
    if (handle != we::rhi::RHIBufferHandle::Invalid) {
        (void)m_Device->DestroyBuffer(handle);
        handle = we::rhi::RHIBufferHandle::Invalid;
        capacity = 0;
    }
    const uint64_t newCapacity = std::max(required * 2, required);
    we::rhi::BufferDesc desc{};
    desc.size = newCapacity;
    desc.usage = usage;
    desc.memory = we::rhi::MemoryUsage::HostVisible;
    desc.debugName = debugName;
    auto buf = m_Device->CreateBuffer(desc);
    if (!buf) {
        return false;
    }
    handle = *buf;
    capacity = newCapacity;
    return true;
}

void UiImmediateRenderer::UpdateGeometryBuffers(
    uint32_t frameSlot,
    const std::vector<we::rhi::UIVertex>& vertices,
    const std::vector<uint32_t>& indices)
{
    if (frameSlot >= m_FrameGeometry.size() || vertices.empty() || indices.empty() || !m_Device) {
        return;
    }
    FrameGeometry& frame = m_FrameGeometry[frameSlot];
    const uint64_t vertexBytes = vertices.size() * sizeof(we::rhi::UIVertex);
    const uint64_t indexBytes = indices.size() * sizeof(uint32_t);

    if (!EnsureBuffer(
            frame.vertexBuffer,
            frame.vertexCapacity,
            vertexBytes,
            we::rhi::BufferUsage::Vertex,
            "UiImmediate.VB")
        || !EnsureBuffer(
            frame.indexBuffer,
            frame.indexCapacity,
            indexBytes,
            we::rhi::BufferUsage::Index,
            "UiImmediate.IB")) {
        return;
    }

    (void)m_Device->UpdateBuffer(
        frame.vertexBuffer,
        std::span(reinterpret_cast<const uint8_t*>(vertices.data()), static_cast<size_t>(vertexBytes)));
    (void)m_Device->UpdateBuffer(
        frame.indexBuffer,
        std::span(reinterpret_cast<const uint8_t*>(indices.data()), static_cast<size_t>(indexBytes)));
}

void UiImmediateRenderer::BeginFrame(const we::rhi::FramePresentParams& params) {
    if (!m_Ready || !m_Device || !params.commandList) {
        return;
    }
    m_Cmd = params.commandList;

    auto* swap = m_Device->GetSwapchain();
    m_CurrentWidth = params.extent.width
        ? params.extent.width
        : (swap ? swap->GetExtent().width : 0);
    m_CurrentHeight = params.extent.height
        ? params.extent.height
        : (swap ? swap->GetExtent().height : 0);
    if (m_CurrentWidth == 0 || m_CurrentHeight == 0) {
        m_Cmd = nullptr;
        return;
    }

    we::rhi::RHITextureHandle color = params.colorTarget;
    if (color == we::rhi::RHITextureHandle::Invalid && swap) {
        color = swap->GetCurrentImage();
    }
    if (color == we::rhi::RHITextureHandle::Invalid) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            "UiImmediateRenderer::BeginFrame: no swapchain/color target.");
        m_Cmd = nullptr;
        return;
    }

    we::rhi::RenderingInfo info{};
    we::rhi::ColorAttachmentDesc colorAtt{};
    colorAtt.texture = color;
    colorAtt.loadOp = we::rhi::LoadOp::Load;
    colorAtt.storeOp = we::rhi::StoreOp::Store;
    info.colorAttachments.push_back(colorAtt);
    info.renderArea = {m_CurrentWidth, m_CurrentHeight};

    m_Cmd->BeginRendering(info);
    m_InRenderPass = true;
    m_Cmd->BindGraphicsPipeline(m_UiPipeline);
    m_BoundSet = we::rhi::RHIDescriptorSetHandle::Invalid;

    if (m_DummySet != we::rhi::RHIDescriptorSetHandle::Invalid) {
        const we::rhi::RHIDescriptorSetHandle sets[] = {m_DummySet};
        m_Cmd->BindDescriptorSets(
            we::rhi::PipelineBindPoint::Graphics, m_UiLayout, 0, sets);
        m_BoundSet = m_DummySet;
    }

    m_Cmd->SetViewport({
        0.0f,
        0.0f,
        static_cast<float>(m_CurrentWidth),
        static_cast<float>(m_CurrentHeight),
        0.0f,
        1.0f});
    m_Cmd->SetScissor({0, 0, m_CurrentWidth, m_CurrentHeight});

    float push[4];
    FillUiTransform(m_CurrentWidth, m_CurrentHeight, push);
    m_Cmd->PushConstants(
        m_UiLayout,
        we::rhi::ShaderStageFlags::Vertex,
        0,
        std::span(reinterpret_cast<const uint8_t*>(push), sizeof(push)));
}

void UiImmediateRenderer::SubmitDrawList(const we::rhi::UIDrawList& list, uint32_t frameSlot) {
    if (!m_Cmd || !m_InRenderPass || frameSlot >= m_FrameGeometry.size()) {
        return;
    }
    if (!list.vertices.empty() && !list.indices.empty()) {
        UpdateGeometryBuffers(frameSlot, list.vertices, list.indices);
    }

    const FrameGeometry& buffers = m_FrameGeometry[frameSlot];
    if (buffers.vertexBuffer == we::rhi::RHIBufferHandle::Invalid
        || buffers.indexBuffer == we::rhi::RHIBufferHandle::Invalid
        || list.batches.empty()) {
        return;
    }

    m_Cmd->BindVertexBuffer(0, buffers.vertexBuffer, 0);
    m_Cmd->BindIndexBuffer(buffers.indexBuffer, 0, we::rhi::IndexType::UInt32);

    const we::rhi::Scissor fullScissor{0, 0, m_CurrentWidth, m_CurrentHeight};

    auto tryDrawBatch = [&](const we::rhi::UIDrawBatch& batch, bool useTextPipeline) -> bool {
        if (batch.texture == we::rhi::RHIDescriptorSetHandle::Invalid || batch.indexCount == 0) {
            return false;
        }
        if (batch.texture == m_DummySet) {
            uint32_t firstVertex = 0;
            if (!list.indices.empty() && batch.firstIndex < list.indices.size()) {
                firstVertex = list.indices[batch.firstIndex];
            }
            const bool samplesTexture = firstVertex < list.vertices.size()
                && (list.vertices[firstVertex].sdfParams[1] < 0.5f
                    || (list.vertices[firstVertex].sdfParams[1] > 3.5f
                        && list.vertices[firstVertex].sdfParams[1] < 4.5f)
                    || (list.vertices[firstVertex].sdfParams[1] > 2.5f
                        && list.vertices[firstVertex].sdfParams[1] < 3.5f));
            if (samplesTexture) {
                return false;
            }
        }

        we::rhi::Scissor batchScissor{
            static_cast<int32_t>(batch.scissor[0]),
            static_cast<int32_t>(batch.scissor[1]),
            static_cast<uint32_t>(batch.scissor[2]),
            static_cast<uint32_t>(batch.scissor[3])};
        if (batchScissor.width == 0 || batchScissor.height == 0) {
            batchScissor = fullScissor;
        }
        m_Cmd->SetScissor(batchScissor);

        const we::rhi::RHIPipelineLayoutHandle layout =
            useTextPipeline ? m_TextLayout : m_UiLayout;

        if (useTextPipeline) {
            TextPushConstants textPush{};
            FillTextPush(
                m_CurrentWidth,
                m_CurrentHeight,
                batch.atlasWidth,
                batch.atlasHeight,
                batch.msdfPixelRange,
                textPush);
            m_Cmd->PushConstants(
                m_TextLayout,
                we::rhi::ShaderStageFlags::Vertex | we::rhi::ShaderStageFlags::Fragment,
                0,
                std::span(reinterpret_cast<const uint8_t*>(&textPush), sizeof(textPush)));
        }

        if (batch.texture != m_BoundSet) {
            const we::rhi::RHIDescriptorSetHandle sets[] = {batch.texture};
            m_Cmd->BindDescriptorSets(
                we::rhi::PipelineBindPoint::Graphics, layout, 0, sets);
            m_BoundSet = batch.texture;
        }

        m_Cmd->DrawIndexed(batch.indexCount, 1, batch.firstIndex, batch.vertexOffset, 0);
        return true;
    };

    for (const auto& batch : list.batches) {
        if (!batch.isText) {
            (void)tryDrawBatch(batch, false);
        }
    }

    bool textPipelineBound = false;
    for (const auto& batch : list.batches) {
        if (!batch.isText) {
            continue;
        }
        if (!textPipelineBound) {
            m_Cmd->BindGraphicsPipeline(m_TextPipeline);
            m_BoundSet = we::rhi::RHIDescriptorSetHandle::Invalid;
            textPipelineBound = true;
        }
        (void)tryDrawBatch(batch, true);
    }
}

void UiImmediateRenderer::EndFrame() {
    if (m_Cmd && m_InRenderPass) {
        m_Cmd->EndRendering();
    }
    m_InRenderPass = false;
    m_Cmd = nullptr;
    m_BoundSet = we::rhi::RHIDescriptorSetHandle::Invalid;
}

} // namespace WindEffects::Editor::UI
