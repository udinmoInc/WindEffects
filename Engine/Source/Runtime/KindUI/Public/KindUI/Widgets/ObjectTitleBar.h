// ==============================================================================
// WindEffects — KindUI — ObjectTitleBar
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace we::runtime::kindui {

/// Reusable title header bar for object inspection across editor tools.
class KINDUI_API ObjectTitleBar : public Row {
public:
    ObjectTitleBar(std::string title = "", WindIconRef icon = kWindIconNone);
    ~ObjectTitleBar() override;

    void SetTitle(std::string title);
    void SetIcon(WindIconRef icon);
    void SetSubtitle(std::string subtitle);

    void SetOnAddClicked(std::function<void()> cb);
    void SetOnBlueprintClicked(std::function<void()> cb);
    void SetOnHelpClicked(std::function<void()> cb);
    void SetOnLockToggled(std::function<void(bool locked)> cb);

    Size Measure(const Size& availableSize) override;
    void Paint(PaintContext& context) override;

private:
    std::string m_Title;
    std::string m_Subtitle;
    WindIconRef m_Icon = kWindIconNone;
    bool m_IsLocked = false;

    std::shared_ptr<ToolbarButton> m_AddBtn;
    std::shared_ptr<IconButton> m_BlueprintBtn;
    std::shared_ptr<IconButton> m_QuestionBtn;
    std::shared_ptr<IconButton> m_LockBtn;

    std::function<void()> m_OnAddClicked;
    std::function<void()> m_OnBlueprintClicked;
    std::function<void()> m_OnHelpClicked;
    std::function<void(bool locked)> m_OnLockToggled;
};

} // namespace we::runtime::kindui
