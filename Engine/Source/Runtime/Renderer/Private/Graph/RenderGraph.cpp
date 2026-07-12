#include "Graph/RenderGraph.h"

#include "Core/Validation.h"
#include "Core/ImageBarriers.h"
#include "Core/AgentDebugLog.h"
#include "Resource/DepthTarget.h"
#include "Graph/FrameContext.h"

#include <unordered_map>
#include <string>
#include <algorithm>

namespace we::runtime::renderer {

RenderGraph::~RenderGraph() {
    Shutdown();
}

void RenderGraph::Init(Renderer* renderer) {
    WE_VALIDATE_INIT(!m_Initialized, "RenderGraph", "Already initialized.");
    WE_VALIDATE_INIT(renderer != nullptr, "RenderGraph", "Renderer is null.");
    
    m_Renderer = renderer;
    m_Initialized = true;
}

void RenderGraph::Shutdown() {
    if (!m_Initialized) return;
    
    m_Passes.clear();
    m_Compiled = false;
    m_Initialized = false;
}

void RenderGraph::AddPass(std::unique_ptr<RenderPass> pass) {
    WE_VALIDATE_INIT(m_Initialized, "RenderGraph", "Cannot add pass to uninitialized RenderGraph.");
    WE_VALIDATE_INIT(!m_Compiled, "RenderGraph", "Cannot add pass to a compiled graph.");
    
    m_Passes.push_back(std::move(pass));
}

void RenderGraph::Compile() {
    WE_VALIDATE_INIT(m_Initialized, "RenderGraph", "Cannot compile uninitialized RenderGraph.");

    // Pass independence: enforce ordering based on declared read/write resources.
    // This is a minimal dependency resolver to prevent accidental pass reordering.
    const size_t n = m_Passes.size();
    if (n == 0) {
        m_Compiled = true;
        return;
    }

    std::vector<RenderPassIO> ios;
    ios.reserve(n);
    for (auto& pass : m_Passes) {
        pass->Validate();
        ios.push_back(pass->DescribePassIO());
    }

    std::vector<std::vector<size_t>> edges(n);
    std::vector<size_t> indegree(n, 0);

    auto addEdge = [&](size_t from, size_t to) {
        edges[from].push_back(to);
        ++indegree[to];
    };

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            if (i == j) continue;

            // Color dependency: writer -> reader.
            if (!ios[i].colorResourceName.empty() &&
                ios[i].color.write &&
                !ios[j].colorResourceName.empty() &&
                ios[i].colorResourceName == ios[j].colorResourceName &&
                ios[j].color.read) {
                addEdge(i, j);
            }

            // Depth dependency: writer -> reader.
            if (!ios[i].depthResourceName.empty() &&
                ios[i].depth.write &&
                !ios[j].depthResourceName.empty() &&
                ios[i].depthResourceName == ios[j].depthResourceName &&
                ios[j].depth.read) {
                addEdge(i, j);
            }
        }
    }

    // Kahn topological sort using original order as tie-breaker.
    std::vector<size_t> ready;
    ready.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        if (indegree[i] == 0) ready.push_back(i);
    }
    std::sort(ready.begin(), ready.end());

    std::vector<size_t> order;
    order.reserve(n);
    while (!ready.empty()) {
        size_t u = ready.front();
        ready.erase(ready.begin());
        order.push_back(u);
        for (size_t v : edges[u]) {
            if (indegree[v] > 0) {
                --indegree[v];
                if (indegree[v] == 0) {
                    ready.push_back(v);
                }
            }
        }
        std::sort(ready.begin(), ready.end());
    }

    if (order.size() != n) {
        // Cyclic / ambiguous dependencies: keep the insertion order.
        m_Compiled = true;
        return;
    }

    // Reorder passes to the computed execution order.
    std::vector<std::unique_ptr<RenderPass>> newPasses;
    newPasses.reserve(n);
    for (size_t idx : order) {
        newPasses.push_back(std::move(m_Passes[idx]));
    }
    m_Passes = std::move(newPasses);

    m_Compiled = true;
}

void RenderGraph::Execute(const FrameContext& frame) {
    WE_VALIDATE_RENDER(m_Initialized, "RenderGraph::Execute", "RenderGraph not initialized.");
    WE_VALIDATE_RENDER(m_Compiled, "RenderGraph::Execute", "RenderGraph not compiled.");

    char dataJson[256];
    std::snprintf(
        dataJson,
        sizeof(dataJson),
        "{"
        "\"frameIndex\":%u,"
        "\"imageIndex\":%u,"
        "\"passCount\":%zu"
        "}",
        frame.frameIndex,
        frame.imageIndex,
        m_Passes.size());

    // #region agent log
    AgentDebugLog(
        "H4",
        "RenderGraph.cpp:Execute",
        "RENDER_GRAPH_EXECUTION",
        dataJson);
    // #endregion

    std::unordered_map<std::string, VkImageLayout> layoutState;
    // Initial assumptions: the renderer transitions the viewport attachments prior to graph execution.
    layoutState["SceneColor"] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    layoutState["SceneDepth"] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    for (auto& pass : m_Passes) {
        // Optional layout transitions based on declared pass IO.
        const RenderPassIO io = pass->DescribePassIO();
        if (!io.colorResourceName.empty() && frame.colorImage != VK_NULL_HANDLE) {
            const VkImageLayout desiredLayout = io.color.requiredLayout;
            const auto it = layoutState.find(io.colorResourceName);
            VkImageLayout currentLayout = (it != layoutState.end()) ? it->second : desiredLayout;

            if (currentLayout != desiredLayout) {
                VkAccessFlags srcAccessMask = 0;
                VkAccessFlags dstAccessMask = 0;
                VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

                if (desiredLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                    dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

                    if (currentLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                        srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    }
                    else {
                        srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    }
                }
                else if (desiredLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                    dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

                    if (currentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                        srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    }
                    else if (currentLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
                        srcAccessMask = 0;
                        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    }
                    else {
                        srcAccessMask = 0;
                        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    }
                }

                TransitionImageLayout(
                    frame.commandBuffer,
                    frame.colorImage,
                    currentLayout,
                    desiredLayout,
                    srcAccessMask,
                    dstAccessMask,
                    srcStage,
                    dstStage);

                layoutState[io.colorResourceName] = desiredLayout;
            }
        }

        if (!io.depthResourceName.empty() && frame.depthTarget != nullptr && frame.depthTarget->GetImage() != VK_NULL_HANDLE) {
            const VkImageLayout desiredLayout = io.depth.requiredLayout;
            const auto it = layoutState.find(io.depthResourceName);
            VkImageLayout currentLayout = (it != layoutState.end()) ? it->second : desiredLayout;

            if (currentLayout != desiredLayout) {
                TransitionDepthImageLayout(
                    frame.commandBuffer,
                    frame.depthTarget->GetImage(),
                    currentLayout,
                    desiredLayout);
                layoutState[io.depthResourceName] = desiredLayout;
            }
        }

        pass->Execute(frame);
    }
}

} // namespace we::runtime::renderer
