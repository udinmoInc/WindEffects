#pragma once
#include <volk.h>

namespace we::editor::rendering {
    struct OverlayRenderContext {
        VkCommandBuffer cmd;
        VkImageView targetView;
        VkFormat targetFormat;
        VkExtent2D targetExtent;
        VkOffset2D viewportOffset{0, 0}; // Offset where viewport is rendered in swapchain
    };
}
