#include "Widgets/StatusBar.h"
#include "Widgets/CommandInput.h"
#include "Widgets/ToolButton.h"
#include "Core/PaintContext.h"
#include "Core/Theme.h"
#include "Core/Icon.h"
#include "Layout/Spacer.h"

namespace we::UI {

namespace {
    class FixedGap : public Widget {
    public:
        explicit FixedGap(float width) : m_Width(width) {}
        Size Measure(const Size& availableSize) override {
            (void)availableSize;
            m_DesiredSize = Size{ m_Width, 1.0f };
            return m_DesiredSize;
        }
        void Arrange(const Rect& allottedRect) override { m_Geometry = allottedRect; }
        void Paint(PaintContext& context) override { (void)context; }
    private:
        float m_Width;
    };

    std::shared_ptr<ToolButton> MakeFooterControl(
        const char* iconName,
        const std::string& label,
        bool isDropdown,
        const char* tooltip)
    {
        auto button = std::make_shared<ToolButton>(iconName, label, nullptr, tooltip);
        button->SetButtonStyle(ToolButtonStyle::ToolbarInline);
        button->SetIsDropdown(isDropdown);
        button->SetVerticalAlignment(VerticalAlignment::Center);
        return button;
    }
}

StatusBar::StatusBar() {
    SetPadding(Margin{ 8.0f, 0.0f, 8.0f, 0.0f });
    SetSpacing(0.0f);
}

void StatusBar::Construct() {
    auto leftBox = std::make_shared<HorizontalBox>();
    leftBox->SetSpacing(4.0f);

    m_AssetsPanelButton = MakeFooterControl(Icons::FolderName, "Assets", false, "Content Browser");
    m_DiagnosticsPanelButton = MakeFooterControl(Icons::WarningName, "Diagnostics", false, "Diagnostics Panel");

    m_AssetsPanelButton->SetOnClicked([this]() { SelectPanelTab(0, true); });
    m_DiagnosticsPanelButton->SetOnClicked([this]() { SelectPanelTab(1, true); });

    leftBox->AddChild(m_AssetsPanelButton);
    leftBox->AddChild(m_DiagnosticsPanelButton);
    AddChild(leftBox);

    AddChild(std::make_shared<FixedGap>(8.0f));

    m_CommandInput = std::make_shared<CommandInput>();
    m_CommandInput->SetVerticalAlignment(VerticalAlignment::Center);
    m_CommandInput->SetPlaceholder("Enter command...");
    AddChild(m_CommandInput);

    AddChild(std::make_shared<Spacer>());

    auto rightBox = std::make_shared<HorizontalBox>();
    rightBox->SetSpacing(4.0f);

    m_OutputLogButton = MakeFooterControl(Icons::ConsoleName, "Output Log", false, "Open Output Log");
    m_BuildMenuButton = MakeFooterControl(Icons::BuildName, "Development", true, "Build Configuration");
    m_TraceButton = MakeFooterControl(Icons::ProfilerName, "Trace", false, "Open Trace Insights");
    m_QualityMenuButton = MakeFooterControl(Icons::LitName, "High", true, "Rendering Quality");

    rightBox->AddChild(m_OutputLogButton);
    rightBox->AddChild(m_BuildMenuButton);
    rightBox->AddChild(std::make_shared<FixedGap>(6.0f));
    rightBox->AddChild(m_TraceButton);
    rightBox->AddChild(m_QualityMenuButton);

    AddChild(rightBox);

    SelectPanelTab(0, false);
}

void StatusBar::SelectPanelTab(int index, bool notify) {
    if (index < 0 || index > 1) {
        return;
    }

    m_ActivePanelTab = index;
    if (m_AssetsPanelButton) {
        m_AssetsPanelButton->SetActive(index == 0);
    }
    if (m_DiagnosticsPanelButton) {
        m_DiagnosticsPanelButton->SetActive(index == 1);
    }

    if (notify && m_OnFooterTabChanged) {
        m_OnFooterTabChanged(index);
    }
}

Size StatusBar::Measure(const Size& availableSize) {
    HorizontalBox::Measure(availableSize);
    m_DesiredSize = Size{ availableSize.width, m_Height };
    return m_DesiredSize;
}

void StatusBar::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, Theme::Get().FooterBackground);

    Rect topBorder{
        m_Geometry.x,
        m_Geometry.y,
        m_Geometry.width,
        1.0f
    };
    context.DrawRect(topBorder, Theme::Get().BorderDefault);

    HorizontalBox::Paint(context);
}

void StatusBar::SetActiveFooterTab(int index) {
    SelectPanelTab(index, false);
}

void StatusBar::SetOnFooterTabChanged(std::function<void(int)> onChanged) {
    m_OnFooterTabChanged = std::move(onChanged);
}

void StatusBar::SetOnCommandSubmitted(std::function<void(const std::string&)> onSubmitted) {
    if (m_CommandInput) {
        m_CommandInput->SetOnCommandSubmitted(std::move(onSubmitted));
    }
}

void StatusBar::SetOnOutputLogClicked(std::function<void()> onClicked) {
    if (m_OutputLogButton) {
        m_OutputLogButton->SetOnClicked(std::move(onClicked));
    }
}

void StatusBar::SetOnBuildMenuClicked(std::function<void()> onClicked) {
    if (m_BuildMenuButton) {
        m_BuildMenuButton->SetOnClicked(std::move(onClicked));
    }
}

void StatusBar::SetOnTraceClicked(std::function<void()> onClicked) {
    if (m_TraceButton) {
        m_TraceButton->SetOnClicked(std::move(onClicked));
    }
}

void StatusBar::SetOnQualityMenuClicked(std::function<void()> onClicked) {
    if (m_QualityMenuButton) {
        m_QualityMenuButton->SetOnClicked(std::move(onClicked));
    }
}

} // namespace we::UI
