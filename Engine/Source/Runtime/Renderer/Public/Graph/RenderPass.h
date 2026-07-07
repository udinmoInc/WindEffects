#pragma once

#include <string>
#include <volk.h>

namespace we::runtime::renderer {

struct FrameContext;

class RenderPass {
public:
    explicit RenderPass(std::string name) : m_Name(std::move(name)) {}
    virtual ~RenderPass() = default;

    const std::string& GetName() const { return m_Name; }

    virtual void Validate() = 0;
    virtual void Execute(const FrameContext& frame) = 0;

private:
    std::string m_Name;
};

} // namespace we::runtime::renderer
