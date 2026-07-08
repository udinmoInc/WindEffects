#pragma once

#include <string>
#include <volk.h>

namespace we::runtime::renderer {

struct FrameContext;

struct AttachmentIO {
    bool read = false;
    bool write = false;
    VkImageLayout requiredLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

struct RenderPassIO {
    std::string colorResourceName;
    AttachmentIO color;
    std::string depthResourceName;
    AttachmentIO depth;
};

class RenderPass {
public:
    explicit RenderPass(std::string name) : m_Name(std::move(name)) {}
    virtual ~RenderPass() = default;

    const std::string& GetName() const { return m_Name; }

    virtual void Validate() = 0;
    virtual void Execute(const FrameContext& frame) = 0;
    virtual RenderPassIO DescribePassIO() const = 0;

private:
    std::string m_Name;
};

} // namespace we::runtime::renderer
