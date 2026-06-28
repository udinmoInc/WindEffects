#pragma once

#include "Layout/Box.hpp"
#include <string>
#include <vector>
#include <functional>

namespace we::UI {

// Status bar widget for application status information
class StatusBar : public HorizontalBox {
public:
    StatusBar();
    virtual ~StatusBar() = default;

    void Construct() override;
    Size Measure(const Size& availableSize) override;
    void Paint(PaintContext& context) override;

    // Quick access
    void SetFPS(float fps);
    void SetGPU(const std::string& name);
    void SetCPU(const std::string& cpu);
    void SetPing(int ms);
    void SetMemory(float gb);
    void SetCompileStatus(const std::string& status);

    void SetHeight(float height) { m_Height = height; }

private:
    float m_Height = 24.0f;

    std::shared_ptr<class Label> m_FPSLabel;
    std::shared_ptr<class Label> m_GPULabel;
    std::shared_ptr<class Label> m_CPULabel;
    std::shared_ptr<class Label> m_MemoryLabel;
    std::shared_ptr<class Label> m_PingLabel;
    std::shared_ptr<class Label> m_CompileLabel;
};

} // namespace we::editor::mainframe::UI
