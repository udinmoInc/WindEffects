#include "Platform/Platform.h"
#include "Widgets/StatusBar.h"
#include "Widgets/CommandInput.h"
#include "Widgets/ToolButton.h"
#include "KindUI/Core/Widgets/VerticalDivider.h"
#include "KindUI/Layout/Spacer.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Profiling/UiGeometryDebug.h"
#include "KindUI/Tokens/SurfaceRole.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Tokens/DesignSystem.h"
#include "KindUI/Theming/ThemeAccess.h"
#include <algorithm>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::Spacer;

namespace we::editor::shell {
using ::we::editor::toolbar::ToolButton;
using ::we::editor::toolbar::ToolButtonStyle;
using ::we::runtime::kindui::VerticalAlignment;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::Margin;
using ::we::runtime::kindui::AlignItems;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;
namespace ControlChrome = ::we::runtime::kindui::ControlChrome;

namespace {

    std::shared_ptr<ToolButton> MakeDockControl(
        we::runtime::kindui::WindIconRef icon,
        const std::string& label,
        const char* tooltip)
    {
        auto button = std::make_shared<ToolButton>(icon, label, nullptr, tooltip);
        button->SetButtonStyle(ToolButtonStyle::StatusBar);
        button->SetVerticalAlignment(VerticalAlignment::Center);
        return button;
    }

    std::shared_ptr<ToolButton> MakeStatusIndicator(
        const std::string& label,
        const char* tooltip)
    {
        auto button = std::make_shared<ToolButton>(kWindIconNone, label, nullptr, tooltip);
        button->SetButtonStyle(ToolButtonStyle::StatusBar);
        button->SetVerticalAlignment(VerticalAlignment::Center);
        return button;
    }

    float UiScale() {
        return std::max(1.0f, DPIContext::GetScale());
    }

    void PaintSectionSeparator(PaintContext& context, float x, float barTop, float barBottom) {
        const float uiScale = UiScale();
        const float lineHeight = we::runtime::kindui::ResolveMetric(MetricToken::ToolbarSeparatorHeight) * uiScale;
        const float centerY = barTop + (barBottom - barTop) * 0.5f;
        const float halfH = lineHeight * 0.5f;
        ControlChrome::PaintVerticalSeparator(
            context,
            x,
            centerY - halfH,
            centerY + halfH,
            we::runtime::kindui::ResolveMetric(MetricToken::BorderWidth),
            ColorToken::Separator);
    }

    std::shared_ptr<we::runtime::kindui::VerticalDivider> MakeStatusDivider() {
        auto divider = std::make_shared<we::runtime::kindui::VerticalDivider>();
        divider->SetFlexShrink(0.0f);
        return divider;
    }

} // namespace

StatusBar::StatusBar()
    : m_Height(we::runtime::kindui::ResolveMetric(MetricToken::StatusBarHeight))
{
}

void StatusBar::Construct() {
    const float uiScale = UiScale();
    const float padH = ThemeMetric(MetricToken::Space2) * uiScale;
    const float sectionGap = we::runtime::kindui::ds::Chrome::SeparationGapWide() * uiScale;
    Padding(Margin{ padH, 0, padH, 0 });
    Gap(sectionGap);
    Align(AlignItems::Center);

    m_LeftBox = std::make_shared<Row>();
    m_LeftBox->Gap(ThemeMetric(MetricToken::Space2) * uiScale);
    m_LeftBox->SetFlexShrink(0.0f);

    m_AssetsPanelButton = MakeDockControl(WindIcons::FolderSearch16, "Content Drawer", "Content Browser");
    m_DiagnosticsPanelButton = MakeDockControl(WindIcons::Logs16, "Output Log", "Output Log");

    m_AssetsPanelButton->SetOnClicked([this]() { SelectPanelTab(0, true); });
    m_DiagnosticsPanelButton->SetOnClicked([this]() { SelectPanelTab(1, true); });

    m_LeftBox->AddChild(m_AssetsPanelButton);
    m_LeftBox->AddChild(m_DiagnosticsPanelButton);
    AddChild(m_LeftBox);
    AddChild(MakeStatusDivider());

    m_CommandInput = std::make_shared<CommandInput>();
    m_CommandInput->SetFlatChrome(true);
    m_CommandInput->SetVerticalAlignment(VerticalAlignment::Center);
    m_CommandInput->SetPlaceholder("Console Commands...");
    m_CommandInput->SetFlexGrow(1.0f);
    m_CommandInput->SetFlexShrink(1.0f);
    AddChild(m_CommandInput);

    auto spacer = std::make_shared<Spacer>();
    spacer->SetFlexGrow(1.0f);
    AddChild(spacer);

    m_RightBox = std::make_shared<Row>();
    m_RightBox->SetFlexShrink(0.0f);

    m_OutputLogButton = MakeStatusIndicator("Source Control", "Source Control");
    m_BuildMenuButton = MakeDockControl(WindIcons::InfoYellow16, "FPS", "Frame Rate");
    m_TraceButton = MakeDockControl(WindIcons::AlertRed16, "Memory", "Memory Usage");
    m_QualityMenuButton = MakeDockControl(WindIcons::ChevronUp16, "RHI", "Graphics API");

    m_RightBox->AddChild(m_OutputLogButton);
    m_RightBox->AddChild(MakeStatusDivider());
    m_RightBox->AddChild(m_BuildMenuButton);
    m_RightBox->AddChild(MakeStatusDivider());
    m_RightBox->AddChild(m_TraceButton);
    m_RightBox->AddChild(MakeStatusDivider());
    m_RightBox->AddChild(m_QualityMenuButton);

    AddChild(MakeStatusDivider());
    AddChild(m_RightBox);

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
    Size size = Row::Measure(availableSize);
    size.height = m_Height;
    m_DesiredSize = size;
    return m_DesiredSize;
}

void StatusBar::Arrange(const Rect& allottedRect) {
    const float barHeight = std::min(m_Height, allottedRect.height);
    const float barY = allottedRect.y + allottedRect.height - barHeight;
    m_Geometry = Rect{ allottedRect.x, barY, allottedRect.width, barHeight };
    Row::Arrange(m_Geometry);
}

void StatusBar::Paint(PaintContext& context) {
    context.PushSurfaceOwner("StatusBar", we::runtime::kindui::SurfaceRole::StatusBar);
    context.DrawSurface(m_Geometry, we::runtime::kindui::SurfaceRole::StatusBar, 0.0f, "StatusBar");

    Row::Paint(context);
    context.PopSurfaceOwner();

    if (we::runtime::kindui::UiGeometryDebug::IsEnabled()) {
        we::runtime::kindui::UiGeometryDebug::Get().TraceRegion(
            "StatusBar",
            m_Geometry,
            "EditorShell",
            0.0f,
            0.0f,
            we::runtime::kindui::ResolveMetric(MetricToken::TextSizeSmall));
    }
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
    m_OnOutputLogClicked = std::move(onClicked);
    if (m_DiagnosticsPanelButton) {
        m_DiagnosticsPanelButton->SetOnClicked([this]() {
            SelectPanelTab(1, true);
            if (m_OnOutputLogClicked) {
                m_OnOutputLogClicked();
            }
        });
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

} // namespace we::editor::shell
