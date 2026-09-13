// ==============================================================================
// WindEffects — KindUI — ObjectTitleBar
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Widgets/ObjectTitleBar.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PropertyPanelChrome.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"

#include <algorithm>

namespace we::runtime::kindui {
namespace {

class TitleLabelWidget final : public Widget {
public:
    TitleLabelWidget(std::string title, WindIconRef icon)
        : m_Title(std::move(title)), m_Icon(icon) {}

    void SetTitle(std::string title) {
        if (m_Title != title) {
            m_Title = std::move(title);
            InvalidatePaint();
        }
    }

    void SetIcon(WindIconRef icon) {
        if (m_Icon.stem != icon.stem || m_Icon.sizePx != icon.sizePx) {
            m_Icon = icon;
            InvalidatePaint();
        }
    }

    Size Measure(const Size& availableSize) override {
        m_DesiredSize = Size{ availableSize.width, LayoutMetrics::UnifiedToolbarRowHeight() };
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
    }

    void Paint(PaintContext& context) override {
        if (!m_Title.empty()) {
            PropertyPanelChrome::PaintObjectHeader(context, m_Geometry, m_Title, m_Icon, true);
        }
    }

private:
    std::string m_Title;
    WindIconRef m_Icon = kWindIconNone;
};

} // namespace

ObjectTitleBar::ObjectTitleBar(std::string title, WindIconRef icon)
    : m_Title(std::move(title)), m_Icon(icon) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float padH = ThemeMetric(MetricToken::Space2) * uiScale;
    Padding(Margin{ padH, 0.0f, padH, 0.0f });
    Gap(ThemeMetric(MetricToken::Space1) * uiScale);
    Align(AlignItems::Center);

    auto labelWidget = std::make_shared<TitleLabelWidget>(m_Title, m_Icon);
    labelWidget->SetFlexGrow(1.0f);
    labelWidget->SetFlexShrink(1.0f);
    AddChild(labelWidget);

    m_AddBtn = std::make_shared<ToolbarButton>("Add", WindIcons::Plus16);
    m_AddBtn->SetFlexShrink(0.0f);
    m_AddBtn->SetOnClicked([this]() {
        if (m_OnAddClicked) m_OnAddClicked();
    });
    AddChild(m_AddBtn);

    m_BlueprintBtn = std::make_shared<IconButton>(WindIcons::Blueprint16);
    m_BlueprintBtn->SetBorderless(true);
    m_BlueprintBtn->SetFlexShrink(0.0f);
    m_BlueprintBtn->SetOnClicked([this]() {
        if (m_OnBlueprintClicked) m_OnBlueprintClicked();
    });
    AddChild(m_BlueprintBtn);

    m_QuestionBtn = std::make_shared<IconButton>(WindIcons::CircleHelp16);
    m_QuestionBtn->SetBorderless(true);
    m_QuestionBtn->SetFlexShrink(0.0f);
    m_QuestionBtn->SetOnClicked([this]() {
        if (m_OnHelpClicked) m_OnHelpClicked();
    });
    AddChild(m_QuestionBtn);

    m_LockBtn = std::make_shared<IconButton>(WindIcons::LockOpen16);
    m_LockBtn->SetBorderless(true);
    m_LockBtn->SetFlexShrink(0.0f);
    m_LockBtn->SetOnClicked([this]() {
        m_IsLocked = !m_IsLocked;
        m_LockBtn->SetActive(m_IsLocked);
        if (m_OnLockToggled) m_OnLockToggled(m_IsLocked);
    });
    AddChild(m_LockBtn);
}

ObjectTitleBar::~ObjectTitleBar() = default;

void ObjectTitleBar::SetTitle(std::string title) {
    m_Title = std::move(title);
    if (!m_Children.empty()) {
        if (auto labelWidget = std::dynamic_pointer_cast<TitleLabelWidget>(m_Children[0])) {
            labelWidget->SetTitle(m_Title);
        }
    }
}

void ObjectTitleBar::SetIcon(WindIconRef icon) {
    m_Icon = icon;
    if (!m_Children.empty()) {
        if (auto labelWidget = std::dynamic_pointer_cast<TitleLabelWidget>(m_Children[0])) {
            labelWidget->SetIcon(m_Icon);
        }
    }
}

void ObjectTitleBar::SetSubtitle(std::string subtitle) {
    m_Subtitle = std::move(subtitle);
}

void ObjectTitleBar::SetOnAddClicked(std::function<void()> cb) {
    m_OnAddClicked = std::move(cb);
}

void ObjectTitleBar::SetOnBlueprintClicked(std::function<void()> cb) {
    m_OnBlueprintClicked = std::move(cb);
}

void ObjectTitleBar::SetOnHelpClicked(std::function<void()> cb) {
    m_OnHelpClicked = std::move(cb);
}

void ObjectTitleBar::SetOnLockToggled(std::function<void(bool locked)> cb) {
    m_OnLockToggled = std::move(cb);
}

Size ObjectTitleBar::Measure(const Size& availableSize) {
    return MeasureWithFixedCross(availableSize, LayoutMetrics::UnifiedToolbarRowHeight());
}

void ObjectTitleBar::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, ResolveColor(ColorToken::PanelBackground));
    Row::Paint(context);
}

} // namespace we::runtime::kindui
