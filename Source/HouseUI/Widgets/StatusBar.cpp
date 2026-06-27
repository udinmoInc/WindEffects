#include "StatusBar.hpp"
#include "../Core/PaintContext.hpp"
#include "../Core/Theme.hpp"
#include "../Core/Icon.hpp"
#include "Label.hpp"
#include "IconWidget.hpp"
#include "../Layout/Spacer.hpp"
#include <iomanip>
#include <sstream>

namespace HouseEngine::UI {

StatusBar::StatusBar() {
    SetPadding(Margin{ 16.0f, 0.0f, 16.0f, 0.0f });
}

void StatusBar::Construct() {
    // Left side
    auto leftBox = std::make_shared<HorizontalBox>();
    leftBox->SetSpacing(24.0f);

    auto createStat = [](const std::string& icon, std::shared_ptr<Label>& labelOut, const std::string& initial) {
        auto box = std::make_shared<HorizontalBox>();
        box->SetSpacing(6.0f);
        if (!icon.empty()) box->AddChild(std::make_shared<IconWidget>(icon, 16.0f));
        labelOut = std::make_shared<Label>(initial);
        labelOut->SetStyle(TextStyle::Small());
        box->AddChild(labelOut);
        return box;
    };

    leftBox->AddChild(createStat(Icons::ListName, m_FPSLabel, "FPS: 60.0"));
    leftBox->AddChild(createStat(Icons::SettingsName, m_GPULabel, "GPU: RTX 4090"));
    leftBox->AddChild(createStat(Icons::InfoName, m_MemoryLabel, "Mem: 1.2 GB"));
    leftBox->AddChild(createStat(Icons::LayersName, m_DrawCallsLabel, "Draws: 142"));

    AddChild(leftBox);

    // Spacer
    AddChild(std::make_shared<Spacer>());

    // Right side
    auto rightBox = std::make_shared<HorizontalBox>();
    rightBox->SetSpacing(24.0f);

    rightBox->AddChild(createStat(Icons::CheckName, m_CompileLabel, "Compile: Success"));
    rightBox->AddChild(createStat(Icons::UndoName, m_GitLabel, "main"));

    AddChild(rightBox);
}

Size StatusBar::Measure(const Size& availableSize) {
    HorizontalBox::Measure(availableSize);
    m_DesiredSize = Size{ availableSize.width, m_Height };
    return m_DesiredSize;
}

void StatusBar::Paint(PaintContext& context) {
    // Draw background
    context.DrawRect(m_Geometry, Theme::Get().WindowBackground); // Or a specific status bar color

    // Draw top separator
    Rect separatorRect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, 1.0f };
    context.DrawRect(separatorRect, Theme::Get().BorderDefault);

    // Draw children
    HorizontalBox::Paint(context);
}

void StatusBar::SetFPS(float fps) {
    if (m_FPSLabel) {
        std::stringstream ss;
        ss << "FPS: " << std::fixed << std::setprecision(1) << fps;
        m_FPSLabel->SetText(ss.str());
    }
}

void StatusBar::SetGPU(const std::string& name) {
    if (m_GPULabel) m_GPULabel->SetText("GPU: " + name);
}

void StatusBar::SetMemory(float gb) {
    if (m_MemoryLabel) {
        std::stringstream ss;
        ss << "Mem: " << std::fixed << std::setprecision(2) << gb << " GB";
        m_MemoryLabel->SetText(ss.str());
    }
}

void StatusBar::SetDrawCalls(int calls) {
    if (m_DrawCallsLabel) m_DrawCallsLabel->SetText("Draws: " + std::to_string(calls));
}

void StatusBar::SetCompileStatus(const std::string& status) {
    if (m_CompileLabel) m_CompileLabel->SetText("Compile: " + status);
}

void StatusBar::SetGitBranch(const std::string& branch) {
    if (m_GitLabel) m_GitLabel->SetText(branch);
}

} // namespace HouseEngine::UI
