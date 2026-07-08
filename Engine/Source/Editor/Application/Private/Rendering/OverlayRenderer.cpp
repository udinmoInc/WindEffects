#include "Rendering/OverlayRenderer.h"
#include "Rendering/UIWidgetAdapter.h"
#include "Rendering/UICompositor.h"
#include "Rendering/UIStateManager.h"
#include "Rendering/FontAtlas.h"
#include "Rendering/IconRenderer.h"
#include "Rendering/UiGpuUpload.h"
#include "Core/Widget.h"
#include "Core/PaintContext.h"
#include "Core/Logger.h"
#include "Core/ImageBarriers.h"
#include "Resource/ResourceManager.h"
#include "Shader/ShaderLibrary.h"
#include "Runtime/Core/AssetRegistry.h"

#include <volk.h>
#include <array>
#include <cstring>
#include <stdexcept>

namespace we::UI {

OverlayRenderer::OverlayRenderer() = default;

OverlayRenderer::~OverlayRenderer() {
    Shutdown();
}

bool OverlayRenderer::Init(VkPhysicalDevice physicalDevice,
                       VkDevice logicalDevice,
                       VkQueue graphicsQueue,
                       uint32_t graphicsQueueFamilyIndex,
                       VkFormat swapchainFormat,
                       uint32_t maxFramesInFlight,
                       we::runtime::renderer::DeviceContext* deviceContext,
                       we::runtime::renderer::ResourceManager* resourceManager) {
    if (!physicalDevice || !logicalDevice || !graphicsQueue || !deviceContext || !resourceManager) {
        HE_ERROR("OverlayRenderer: Invalid initialization parameters");
        return false;
    }

    m_PhysicalDevice = physicalDevice;
    m_LogicalDevice = logicalDevice;
    m_GraphicsQueue = graphicsQueue;
    m_GraphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
    m_SwapchainFormat = swapchainFormat;
    m_MaxFramesInFlight = maxFramesInFlight;
    m_DeviceContext = deviceContext;
    m_ResourceManager = resourceManager;

    HE_INFO("OverlayRenderer: Starting Volk Init");
    if (volkInitialize() != VK_SUCCESS) {
        HE_ERROR("OverlayRenderer: Failed to initialize Volk for this module");
        return false;
    }
    HE_INFO("OverlayRenderer: Volk Load Instance");
    volkLoadInstance(deviceContext->GetInstance());
    HE_INFO("OverlayRenderer: Volk Load Device");
    volkLoadDevice(logicalDevice);
    if (!vkCreateDescriptorSetLayout || !vkCreateCommandPool) {
        HE_ERROR("OverlayRenderer: Vulkan function pointers were not loaded");
        return false;
    }

    HE_INFO("OverlayRenderer: Init UiGpuUpload");
    m_GpuUpload = std::make_unique<UiGpuUpload>();
    m_GpuUpload->Init(deviceContext, resourceManager);

    HE_INFO("OverlayRenderer: Create Descriptor Resources");
    CreateDescriptorSetLayouts();
    CreateDescriptorPool();
    if (m_TextureDescriptorSetLayout == VK_NULL_HANDLE || m_DescriptorPool == VK_NULL_HANDLE) {
        HE_ERROR("OverlayRenderer: Failed to create descriptor resources");
        return false;
    }

    m_FontAtlas = std::make_unique<FontAtlas>();
    const std::string uiFontPath = we::core::AssetRegistry::Get().GetFontPath("Font_UI");
    const std::string fontToLoad = uiFontPath.empty() ? "Assets/Fonts/Inter-Regular.ttf" : uiFontPath;
    if (!m_FontAtlas->Init(deviceContext, resourceManager, m_GpuUpload.get(), fontToLoad, 32, 96, 1024, 1024)) {
        HE_ERROR("OverlayRenderer: Font atlas initialization failed");
        return false;
    }

    m_IconRenderer = std::make_unique<IconRenderer>();
    if (!m_IconRenderer->Init(deviceContext, resourceManager, m_GpuUpload.get(), m_DescriptorPool, m_TextureDescriptorSetLayout)) {
        HE_ERROR("OverlayRenderer: IconRenderer initialization failed");
        return false;
    }

    CreateDummyTexture();

    m_FontAtlas->GetDescriptorSetRef() = RegisterTexture(m_FontAtlas->GetImageView(), m_FontAtlas->GetSampler());
    if (m_FontAtlas->GetDescriptorSet() == VK_NULL_HANDLE) {
        HE_ERROR("OverlayRenderer: Failed to allocate font atlas descriptor set");
        return false;
    }

    we::core::AssetRegistry::Get().RegisterTexture("Font_UI_Atlas", m_FontAtlas->GetImageView(), m_FontAtlas->GetSampler());
    we::core::AssetRegistry::Get().RegisterTexture("UI_DummyWhite", m_DummyImageView, m_DummySampler);

    CreateGraphicsPipeline(swapchainFormat);
    if (m_GraphicsPipeline == VK_NULL_HANDLE || m_PipelineLayout == VK_NULL_HANDLE) {
        HE_ERROR("OverlayRenderer: Failed to create graphics pipeline");
        return false;
    }

    m_Compositor = std::make_unique<UICompositor>();
    if (!m_Compositor->Initialize(logicalDevice, physicalDevice, swapchainFormat)) {
        HE_ERROR("OverlayRenderer: Failed to initialize compositor");
        return false;
    }

    m_StateManager = std::make_unique<UIStateManager>();
    m_StateManager->Initialize(logicalDevice);

    m_WidgetAdapter = std::make_unique<UIWidgetAdapter>();
    m_WidgetAdapter->Initialize(this);

    m_FrameGeometry.resize(maxFramesInFlight);

    HE_INFO("OverlayRenderer: Independent UI renderer initialized");
    return true;
}

void OverlayRenderer::Shutdown() {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_WidgetAdapter) {
        m_WidgetAdapter->Shutdown();
        m_WidgetAdapter.reset();
    }
    if (m_Compositor) {
        m_Compositor->Shutdown();
        m_Compositor.reset();
    }
    if (m_StateManager) {
        m_StateManager->Shutdown();
        m_StateManager.reset();
    }
    if (m_IconRenderer) {
        m_IconRenderer->Shutdown();
        m_IconRenderer.reset();
    }
    if (m_FontAtlas) {
        m_FontAtlas->Shutdown();
        m_FontAtlas.reset();
    }

    if (m_LogicalDevice != VK_NULL_HANDLE) {
        if (m_GraphicsPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_LogicalDevice, m_GraphicsPipeline, nullptr);
            m_GraphicsPipeline = VK_NULL_HANDLE;
        }
        if (m_PipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, nullptr);
            m_PipelineLayout = VK_NULL_HANDLE;
        }
        if (m_TextureDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_LogicalDevice, m_TextureDescriptorSetLayout, nullptr);
            m_TextureDescriptorSetLayout = VK_NULL_HANDLE;
        }
        if (m_DescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_LogicalDevice, m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
        }
        if (m_DummySampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_LogicalDevice, m_DummySampler, nullptr);
            m_DummySampler = VK_NULL_HANDLE;
        }
        if (m_DummyImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_LogicalDevice, m_DummyImageView, nullptr);
            m_DummyImageView = VK_NULL_HANDLE;
        }
        if (m_DummyImage != VK_NULL_HANDLE) {
            vkDestroyImage(m_LogicalDevice, m_DummyImage, nullptr);
            m_DummyImage = VK_NULL_HANDLE;
        }
        if (m_DummyMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_LogicalDevice, m_DummyMemory, nullptr);
            m_DummyMemory = VK_NULL_HANDLE;
        }

        for (auto& frame : m_FrameGeometry) {
            if (frame.vertexBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(m_LogicalDevice, frame.vertexBuffer, nullptr);
                frame.vertexBuffer = VK_NULL_HANDLE;
            }
            if (frame.vertexMemory != VK_NULL_HANDLE) {
                vkFreeMemory(m_LogicalDevice, frame.vertexMemory, nullptr);
                frame.vertexMemory = VK_NULL_HANDLE;
            }
            if (frame.indexBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(m_LogicalDevice, frame.indexBuffer, nullptr);
                frame.indexBuffer = VK_NULL_HANDLE;
            }
            if (frame.indexMemory != VK_NULL_HANDLE) {
                vkFreeMemory(m_LogicalDevice, frame.indexMemory, nullptr);
                frame.indexMemory = VK_NULL_HANDLE;
            }
        }
    }

    if (m_GpuUpload) {
        m_GpuUpload->Shutdown();
        m_GpuUpload.reset();
    }

    m_FrameGeometry.clear();
    m_Vertices.clear();
    m_Indices.clear();
    m_Batches.clear();
    m_DeviceContext = nullptr;
    m_ResourceManager = nullptr;
}

void OverlayRenderer::RenderEditorUI(const std::shared_ptr<we::UI::Widget>& root) {
    uint32_t width = m_CurrentWidth;
    uint32_t height = m_CurrentHeight;

    if (!root || width == 0 || height == 0) {
        m_Vertices.clear();
        m_Indices.clear();
        m_Batches.clear();
        return;
    }

    HE_INFO("==========================");
    HE_INFO("UI BUILD");
    HE_INFO("==========================");
    HE_INFO("PASS | UI Tick"); // already done in Editor.cpp effectively
    HE_INFO("PASS | Root Widget Exists");
    if (m_WidgetAdapter) {
        HE_INFO("PASS | OverlayManager Exists (Adapter)");
    }
    
    // We assume width/height is root geometry
    HE_INFO(std::string("PASS | Root Geometry: ") + std::to_string(width) + "x" + std::to_string(height));

    we::UI::Widget::ResetDiagnostics();
    we::UI::UIWidgetAdapter::ResetDiagnostics();

    HE_INFO("PASS | Paint() Started");
    
    if (m_WidgetAdapter) {
        m_WidgetAdapter->ProcessWidget(root, width, height);
        
        m_Vertices = m_WidgetAdapter->GetVertices();
        m_Indices = m_WidgetAdapter->GetIndices();
        m_Batches = m_WidgetAdapter->GetBatches();
    }
    
    HE_INFO("PASS | Paint() Finished");
    
    HE_INFO(std::string("Root Widget Name: Root"));
    HE_INFO(std::string("Total Widget Count: ") + std::to_string(we::UI::Widget::s_TotalWidgetCount));
    HE_INFO(std::string("Visible Widget Count: ") + std::to_string(we::UI::Widget::s_VisibleWidgetCount));
    HE_INFO(std::string("Hidden Widget Count: ") + std::to_string(we::UI::Widget::s_HiddenWidgetCount));
    HE_INFO(std::string("Paint() Calls: ") + std::to_string(we::UI::Widget::s_PaintCalls));
    HE_INFO(std::string("ArrangeChildren() Calls: ") + std::to_string(we::UI::Widget::s_ArrangeChildrenCalls));
    HE_INFO(std::string("Layout Pass Count: ") + std::to_string(we::UI::Widget::s_LayoutPassCount));
    HE_INFO(std::string("Invalidate Count: ") + std::to_string(we::UI::Widget::s_InvalidateCount));

    HE_INFO("==========================");
    HE_INFO("DRAW COMMAND GENERATION");
    HE_INFO("==========================");
    HE_INFO(std::string("Total Draw Commands Generated: ") + std::to_string(we::UI::UIWidgetAdapter::s_TotalDrawCommandsGenerated));
    HE_INFO(std::string("Rectangle Commands: ") + std::to_string(we::UI::UIWidgetAdapter::s_RectangleCommands));
    HE_INFO(std::string("Text Commands: ") + std::to_string(we::UI::UIWidgetAdapter::s_TextCommands));
    HE_INFO(std::string("Image Commands: ") + std::to_string(we::UI::UIWidgetAdapter::s_ImageCommands));
    HE_INFO(std::string("Icon Commands: ") + std::to_string(we::UI::UIWidgetAdapter::s_IconCommands));
    HE_INFO(std::string("Border Commands: ") + std::to_string(we::UI::UIWidgetAdapter::s_BorderCommands));
    HE_INFO(std::string("Gradient Commands: ") + std::to_string(we::UI::UIWidgetAdapter::s_GradientCommands));
    HE_INFO(std::string("Shadow Commands: ") + std::to_string(we::UI::UIWidgetAdapter::s_ShadowCommands));
    HE_INFO(std::string("Clip Rect Count: ") + std::to_string(we::UI::UIWidgetAdapter::s_ClipRectCount));
    HE_INFO(std::string("Texture Switch Count: ") + std::to_string(we::UI::UIWidgetAdapter::s_TextureSwitchCount));
    HE_INFO(std::string("Batch Count: ") + std::to_string(we::UI::UIWidgetAdapter::s_BatchCount));
    
    if (we::UI::UIWidgetAdapter::s_TotalDrawCommandsGenerated == 0) {
        HE_ERROR("STOPPING: DrawCommands == 0! Paint() produced nothing.");
        m_Vertices.clear();
        m_Indices.clear();
        m_Batches.clear();
        return;
    }

    HE_INFO("==========================");
    HE_INFO("BUILD GEOMETRY");
    HE_INFO("==========================");
    HE_INFO(std::string("Input DrawCommand Count: ") + std::to_string(we::UI::UIWidgetAdapter::s_TotalDrawCommandsGenerated));
    HE_INFO(std::string("Generated Vertices: ") + std::to_string(m_Vertices.size()));
    HE_INFO(std::string("Generated Indices: ") + std::to_string(m_Indices.size()));
    HE_INFO(std::string("Generated Batches: ") + std::to_string(m_Batches.size()));
    HE_INFO(std::string("Vertex Buffer Size: ") + std::to_string(m_Vertices.size() * sizeof(UIVertex2)));
    HE_INFO(std::string("Index Buffer Size: ") + std::to_string(m_Indices.size() * sizeof(uint32_t)));

    if (we::UI::UIWidgetAdapter::s_TotalDrawCommandsGenerated > 0 && m_Vertices.size() <= 4) {
        HE_ERROR("STOPPING: DrawCommands > 0 but Vertices <= 4!");
    }

    HE_INFO("==========================");
    HE_INFO("FINAL CHECK");
    HE_INFO("==========================");
    HE_INFO(std::string("Expected Vertex Count: ") + std::to_string(we::UI::UIWidgetAdapter::s_TotalDrawCommandsGenerated * 4));
    HE_INFO(std::string("Actual Vertex Count: ") + std::to_string(m_Vertices.size()));
    HE_INFO(std::string("Expected DrawCommands: ") + std::to_string(we::UI::UIWidgetAdapter::s_TotalDrawCommandsGenerated));
    HE_INFO(std::string("Actual DrawCommands: ") + std::to_string(we::UI::UIWidgetAdapter::s_TotalDrawCommandsGenerated));
    HE_INFO(std::string("Expected Widgets Painted: ") + std::to_string(we::UI::Widget::s_TotalWidgetCount));
    HE_INFO(std::string("Actual Widgets Painted: ") + std::to_string(we::UI::Widget::s_WidgetsPainted));

    if (m_Vertices.size() <= 4) {
        m_Vertices.clear();
        m_Indices.clear();
        m_Batches.clear();
        return;
    }

    m_FrameStats.vertices = static_cast<uint32_t>(m_Vertices.size());
    m_FrameStats.indices = static_cast<uint32_t>(m_Indices.size());
    m_FrameStats.batches = static_cast<uint32_t>(m_Batches.size());

    UpdateGeometryBuffers(m_CurrentFrameIndex);
    HE_INFO(std::string("PASS | Vertex Buffer Upload"));
    HE_INFO(std::string("PASS | Index Buffer Upload"));
}

void OverlayRenderer::BeginOverlayPass(const we::editor::rendering::OverlayRenderContext& context) {
    m_Mutex.lock(); // Ensure we unlock in EndOverlayPass

    HE_INFO("PASS | BeginOverlayPass()");
    HE_INFO("PASS | vkCmdBeginRendering() executed (via Compositor)");

    m_CurrentWidth = context.targetExtent.width;
    m_CurrentHeight = context.targetExtent.height;

    m_StateManager->SaveState(context.cmd, m_SavedState);

    m_Compositor->BeginComposite(context.cmd, context.targetView, m_CurrentWidth, m_CurrentHeight, VK_ATTACHMENT_LOAD_OP_LOAD);

    vkCmdBindPipeline(context.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
    HE_INFO(std::string("PASS | vkCmdBindPipeline() succeed | Pipeline Handle: ") + std::to_string(reinterpret_cast<uint64_t>(m_GraphicsPipeline)));

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_CurrentWidth);
    viewport.height = static_cast<float>(m_CurrentHeight);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(context.cmd, 0, 1, &viewport);
    HE_INFO(std::string("PASS | Viewport: 0,0 to ") + std::to_string(m_CurrentWidth) + "x" + std::to_string(m_CurrentHeight));

    VkRect2D fullScissor{};
    fullScissor.offset = {0, 0};
    fullScissor.extent = {m_CurrentWidth, m_CurrentHeight};
    vkCmdSetScissor(context.cmd, 0, 1, &fullScissor);

    float pushConstants[4];
    pushConstants[0] = 2.0f / static_cast<float>(m_CurrentWidth);
    pushConstants[1] = 2.0f / static_cast<float>(m_CurrentHeight);
    pushConstants[2] = -1.0f;
    pushConstants[3] = -1.0f;
    vkCmdPushConstants(context.cmd, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 4, pushConstants);


}

void OverlayRenderer::EndOverlayPass(const we::editor::rendering::OverlayRenderContext& context) {
    HE_INFO("==========================");
    HE_INFO("GPU SUBMISSION (EndOverlayPass)");
    HE_INFO("==========================");
    HE_INFO(std::string("CPU Batch Count: ") + std::to_string(m_Batches.size()));

    uint32_t gpuBatchCount = 0;
    uint32_t executedDrawCalls = 0;
    uint32_t skippedDrawCalls = 0;

    if (m_CurrentFrameIndex < m_FrameGeometry.size()) {
        const FrameGeometry& buffers = m_FrameGeometry[m_CurrentFrameIndex];
        
        HE_INFO(std::string("Frame Geometry Index: ") + std::to_string(m_CurrentFrameIndex));
        HE_INFO(std::string("Vertex Buffer Handle: ") + std::to_string(reinterpret_cast<uint64_t>(buffers.vertexBuffer)));
        HE_INFO(std::string("Index Buffer Handle: ") + std::to_string(reinterpret_cast<uint64_t>(buffers.indexBuffer)));

        if (buffers.vertexBuffer != VK_NULL_HANDLE && buffers.indexBuffer != VK_NULL_HANDLE && !m_Batches.empty()) {
            VkBuffer vertexBuffers[] = {buffers.vertexBuffer};
            VkDeviceSize vertexOffsets[] = {0};
            vkCmdBindVertexBuffers(context.cmd, 0, 1, vertexBuffers, vertexOffsets);
            vkCmdBindIndexBuffer(context.cmd, buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            HE_INFO("PASS | Bind Vertex Buffer");
            HE_INFO("PASS | Bind Index Buffer");

            for (size_t i = 0; i < m_Batches.size(); ++i) {
                const auto& batch = m_Batches[i];
                gpuBatchCount++;

                VkDescriptorSet textureSet = batch.textureSet != VK_NULL_HANDLE ? batch.textureSet : m_DummyDescriptorSet;
                if (textureSet == VK_NULL_HANDLE) {
                    skippedDrawCalls++;
                    HE_WARN(std::string("Skipped Batch ") + std::to_string(i) + " | Reason: Invalid Texture");
                    continue;
                }
                if (batch.indexCount == 0) {
                    skippedDrawCalls++;
                    HE_WARN(std::string("Skipped Batch ") + std::to_string(i) + " | Reason: IndexCount == 0");
                    continue;
                }

                VkRect2D batchScissor{};
                batchScissor.offset.x = static_cast<int32_t>(batch.scissor[0]);
                batchScissor.offset.y = static_cast<int32_t>(batch.scissor[1]);
                batchScissor.extent.width = static_cast<uint32_t>(batch.scissor[2]);
                batchScissor.extent.height = static_cast<uint32_t>(batch.scissor[3]);
                
                VkRect2D fullScissor{};
                fullScissor.offset = {0, 0};
                fullScissor.extent = {m_CurrentWidth, m_CurrentHeight};
                if (batchScissor.extent.width == 0 || batchScissor.extent.height == 0) {
                    batchScissor = fullScissor;
                }

                vkCmdSetScissor(context.cmd, 0, 1, &batchScissor);
                vkCmdBindDescriptorSets(context.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &textureSet, 0, nullptr);
                vkCmdDrawIndexed(context.cmd, batch.indexCount, 1, batch.firstIndex, batch.vertexOffset, 0);
                
                executedDrawCalls++;
                
                // Detailed batch log
                HE_INFO(std::string("Draw Call [") + std::to_string(i) + "] | " +
                    "Indices: " + std::to_string(batch.indexCount) + " | " +
                    "FirstIdx: " + std::to_string(batch.firstIndex) + " | " +
                    "VOffset: " + std::to_string(batch.vertexOffset) + " | " +
                    "Scissor: " + std::to_string(batchScissor.extent.width) + "x" + std::to_string(batchScissor.extent.height) + " | " +
                    "Desc: " + std::to_string(reinterpret_cast<uint64_t>(textureSet)));
            }
        } else {
            HE_ERROR("STOPPING: Geometry buffers are null or batches are empty!");
            if (buffers.vertexBuffer == VK_NULL_HANDLE) HE_ERROR("Reason: Vertex Buffer is NULL");
            if (buffers.indexBuffer == VK_NULL_HANDLE) HE_ERROR("Reason: Index Buffer is NULL");
            if (m_Batches.empty()) HE_ERROR("Reason: m_Batches is empty");
        }
    } else {
        HE_ERROR("STOPPING: Invalid Frame Index");
    }

    HE_INFO(std::string("GPU Batch Count: ") + std::to_string(gpuBatchCount));
    HE_INFO(std::string("Recorded Draw Calls: ") + std::to_string(executedDrawCalls));
    HE_INFO(std::string("Executed Draw Calls: ") + std::to_string(executedDrawCalls));
    HE_INFO(std::string("Skipped Draw Calls: ") + std::to_string(skippedDrawCalls));

    m_Compositor->EndComposite(context.cmd);
    HE_INFO("PASS | vkCmdEndRendering() executed (via Compositor)");
    m_StateManager->RestoreState(context.cmd, m_SavedState);

    m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFramesInFlight;

    m_Mutex.unlock();
}

VkDescriptorSet OverlayRenderer::RegisterTexture(VkImageView imageView, VkSampler sampler) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_DescriptorPool == VK_NULL_HANDLE || m_TextureDescriptorSetLayout == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_TextureDescriptorSetLayout;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_LogicalDevice, 1, &descriptorWrite, 0, nullptr);
    return descriptorSet;
}

void OverlayRenderer::UpdateTexture(VkDescriptorSet descriptorSet, VkImageView imageView, VkSampler sampler) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (descriptorSet == VK_NULL_HANDLE) {
        return;
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_LogicalDevice, 1, &descriptorWrite, 0, nullptr);
}

void OverlayRenderer::UnregisterTexture(VkDescriptorSet descriptorSet) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (descriptorSet != VK_NULL_HANDLE && m_DescriptorPool != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(m_LogicalDevice, m_DescriptorPool, 1, &descriptorSet);
    }
}

FontAtlas* OverlayRenderer::GetFontAtlas() const {
    return m_FontAtlas.get();
}

IconRenderer* OverlayRenderer::GetIconRenderer() const {
    return m_IconRenderer.get();
}

void OverlayRenderer::CreateDescriptorSetLayouts() {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutInfo, nullptr, &m_TextureDescriptorSetLayout) != VK_SUCCESS) {
        HE_ERROR("OverlayRenderer: Failed to create descriptor set layout");
        m_TextureDescriptorSetLayout = VK_NULL_HANDLE;
    }
}

void OverlayRenderer::CreateDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 4096;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 2048;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
        HE_ERROR("OverlayRenderer: Failed to create descriptor pool");
        m_DescriptorPool = VK_NULL_HANDLE;
    }
}

void OverlayRenderer::CreateDummyTexture() {
    VkDevice device = m_LogicalDevice;
    std::array<uint8_t, 4> pixel = {255, 255, 255, 255};

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    m_ResourceManager->CreateBuffer(
        4,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

    void* data = nullptr;
    vkMapMemory(device, stagingMemory, 0, 4, 0, &data);
    std::memcpy(data, pixel.data(), 4);
    vkUnmapMemory(device, stagingMemory);

    m_ResourceManager->CreateImage(
        1, 1,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_DummyImage,
        m_DummyMemory);

    m_GpuUpload->SubmitOneTime([&](VkCommandBuffer uploadCmd) {
        we::runtime::renderer::TransitionImageLayout(
            uploadCmd,
            m_DummyImage,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {1, 1, 1};
        vkCmdCopyBufferToImage(uploadCmd, stagingBuffer, m_DummyImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        we::runtime::renderer::TransitionImageLayout(
            uploadCmd,
            m_DummyImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    });

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    m_DummyImageView = m_ResourceManager->CreateImageView(m_DummyImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    vkCreateSampler(device, &samplerInfo, nullptr, &m_DummySampler);

    m_DummyDescriptorSet = RegisterTexture(m_DummyImageView, m_DummySampler);
}

void OverlayRenderer::CreateGraphicsPipeline(VkFormat colorFormat) {
    VkDevice device = m_LogicalDevice;

    VkShaderModule vertShaderModule = we::runtime::renderer::ShaderLibrary::Get().CreateShaderModule(
        device, "UI", we::runtime::renderer::ShaderStage::Vertex);
    VkShaderModule fragShaderModule = we::runtime::renderer::ShaderLibrary::Get().CreateShaderModule(
        device, "UI", we::runtime::renderer::ShaderStage::Pixel);

    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        HE_ERROR("OverlayRenderer: Failed to load UI shader modules");
        return;
    }

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "VSMain";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "PSMain";

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(UIVertex2);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
    attributeDescriptions[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UIVertex2, position)};
    attributeDescriptions[1] = {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(UIVertex2, uv)};
    attributeDescriptions[2] = {0, 2, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(UIVertex2, color)};
    attributeDescriptions[3] = {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(UIVertex2, sdfRect)};
    attributeDescriptions[4] = {0, 4, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(UIVertex2, sdfParams)};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 4;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_TextureDescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        HE_ERROR("OverlayRenderer: Failed to create pipeline layout");
        return;
    }

    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &colorFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &pipelineRenderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS) {
        HE_ERROR("OverlayRenderer: Failed to create graphics pipeline");
        m_GraphicsPipeline = VK_NULL_HANDLE;
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void OverlayRenderer::UpdateGeometryBuffers(uint32_t frameIndex) {
    if (frameIndex >= m_FrameGeometry.size() || m_Vertices.empty() || m_Indices.empty() || !m_ResourceManager) {
        return;
    }

    FrameGeometry& frame = m_FrameGeometry[frameIndex];
    VkDevice device = m_LogicalDevice;

    const VkDeviceSize vertexSize = static_cast<VkDeviceSize>(m_Vertices.size() * sizeof(UIVertex2));
    const VkDeviceSize indexSize = static_cast<VkDeviceSize>(m_Indices.size() * sizeof(uint32_t));

    if (vertexSize > frame.vertexSize) {
        if (frame.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, frame.vertexBuffer, nullptr);
            vkFreeMemory(device, frame.vertexMemory, nullptr);
        }
        frame.vertexSize = vertexSize * 2;
        m_ResourceManager->CreateBuffer(
            frame.vertexSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            frame.vertexBuffer,
            frame.vertexMemory);
    }

    if (indexSize > frame.indexSize) {
        if (frame.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, frame.indexBuffer, nullptr);
            vkFreeMemory(device, frame.indexMemory, nullptr);
        }
        frame.indexSize = indexSize * 2;
        m_ResourceManager->CreateBuffer(
            frame.indexSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            frame.indexBuffer,
            frame.indexMemory);
    }

    void* mapped = nullptr;
    vkMapMemory(device, frame.vertexMemory, 0, vertexSize, 0, &mapped);
    std::memcpy(mapped, m_Vertices.data(), static_cast<size_t>(vertexSize));
    vkUnmapMemory(device, frame.vertexMemory);

    vkMapMemory(device, frame.indexMemory, 0, indexSize, 0, &mapped);
    std::memcpy(mapped, m_Indices.data(), static_cast<size_t>(indexSize));
    vkUnmapMemory(device, frame.indexMemory);
}

} // namespace we::UI
