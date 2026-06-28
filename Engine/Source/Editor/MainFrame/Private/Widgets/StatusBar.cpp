#include "StatusBar.hpp"
#include "../Core/PaintContext.hpp"
#include "../Core/Theme.hpp"
#include "../Core/Icon.hpp"
#include "Label.hpp"
#include "IconWidget.hpp"
#include "../Layout/Spacer.hpp"
#include <iomanip>
#include <sstream>

namespace we::UI {

StatusBar::StatusBar() {
    SetPadding(Margin{ 8.0f, 0.0f, 8.0f, 0.0f }); // 8px padding
}

void StatusBar::Construct() {
    // Left side
    auto leftBox = std::make_shared<HorizontalBox>();
    leftBox->SetSpacing(8.0f);
    AddChild(leftBox);

    // Spacer to push right box
    AddChild(std::make_shared<Spacer>());

    // Right side
    auto rightBox = std::make_shared<HorizontalBox>();
    rightBox->SetSpacing(8.0f); // 8px spacing

    auto createStat = [](std::shared_ptr<Label>& labelOut, const std::string& initial) {
        labelOut = std::make_shared<Label>(initial);
        
        TextStyle statStyle;
        statStyle.size = Theme::Get().TextSizeProperty; // 12px
        statStyle.color = Theme::Get().TextSecondary;   // Secondary gray
        labelOut->SetStyle(statStyle);
        
        return labelOut;
    };

    rightBox->AddChild(createStat(m_FPSLabel, "FPS: 60"));
    rightBox->AddChild(createStat(m_GPULabel, "GPU: RTX 4090"));
    rightBox->AddChild(createStat(m_CPULabel, "CPU: 12ms"));
    rightBox->AddChild(createStat(m_MemoryLabel, "Mem: 1.2 GB"));
    rightBox->AddChild(createStat(m_PingLabel, "Ping: 24ms"));
    rightBox->AddChild(createStat(m_CompileLabel, "Compile: OK"));

    AddChild(rightBox);
}

Size StatusBar::Measure(const Size& availableSize) {
    HorizontalBox::Measure(availableSize);
    m_DesiredSize = Size{ availableSize.width, m_Height };
    return m_DesiredSize;
}

void StatusBar::Paint(PaintContext& context) {
    // Draw background
    context.DrawRect(m_Geometry, Theme::Get().WindowBackground); 
    
    // Draw subtle top border
    Rect topBorder{ m_Geometry.x, m_Geometry.y, m_Geometry.width, 1.0f };
    context.DrawRect(topBorder, Theme::Get().BorderDefault);
    
    HorizontalBox::Paint(context);
}

void StatusBar::SetFPS(float fps) {
    if (m_FPSLabel) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << "FPS: " << fps;
        m_FPSLabel->SetText(ss.str());
    }
}

void StatusBar::SetGPU(const std::string& name) {
    if (m_GPULabel) m_GPULabel->SetText("GPU: " + name);
}

void StatusBar::SetCPU(const std::string& cpu) {
    if (m_CPULabel) m_CPULabel->SetText("CPU: " + cpu);
}

void StatusBar::SetMemory(float gb) {
    if (m_MemoryLabel) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << "Mem: " << gb << " GB";
        m_MemoryLabel->SetText(ss.str());
    }
}

void StatusBar::SetPing(int ms) {
    if (m_PingLabel) m_PingLabel->SetText("Ping: " + std::to_string(ms) + "ms");
}

void StatusBar::SetCompileStatus(const std::string& status) {
    if (m_CompileLabel) m_CompileLabel->SetText("Compile: " + status);
}

} // namespace we::editor::mainframe::UI
