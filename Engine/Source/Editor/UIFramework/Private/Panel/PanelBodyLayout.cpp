#include "WindEffects/Editor/UI/Panel/PanelBodyLayout.h"

#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Layout/LayoutAssert.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Tokens/ChromeSeparation.h"
#include "KindUI/Tokens/DesignSystem.h"

#include <algorithm>

namespace we::editor::panels {
namespace Chrome = PanelChrome;
using ::we::runtime::kindui::AssertLayoutRectValid;
using ::we::runtime::kindui::ClampRectToParent;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Widget;

namespace {

constexpr size_t RegionIndex(PanelBodyRegion region) {
    return static_cast<size_t>(region);
}

float RegionSeparationGap() {
    if (we::runtime::kindui::ChromeSeparation::kGapCutsEnabled) {
        return we::runtime::kindui::ChromeSeparation::Gap();
    }
    return 0.0f;
}

void PaintRegionBackground(PanelBodyRegion region, PaintContext& context, const Rect& geometry) {
    switch (region) {
    case PanelBodyRegion::ModeTabs:
    case PanelBodyRegion::ColumnHeader:
    case PanelBodyRegion::Footer:
        Chrome::PaintListLabelBand(context, geometry);
        break;
    case PanelBodyRegion::Search:
    case PanelBodyRegion::Toolbar:
        Chrome::PaintToolbarRegion(context, geometry);
        break;
    case PanelBodyRegion::Content:
        Chrome::PaintContentWell(context, geometry);
        break;
    case PanelBodyRegion::Count:
        break;
    }
}

void PaintRegionChrome(PanelBodyRegion region, PaintContext& context, const Rect& geometry) {
    if (we::runtime::kindui::ChromeSeparation::kGapCutsEnabled) {
        (void)region;
        (void)context;
        (void)geometry;
        return;
    }
    switch (region) {
    case PanelBodyRegion::ModeTabs:
    case PanelBodyRegion::Toolbar:
        Chrome::PaintDockTabStripDivider(context, geometry);
        break;
    case PanelBodyRegion::Search:
        break;
    case PanelBodyRegion::ColumnHeader:
        Chrome::PaintDockTabStripDivider(context, geometry);
        break;
    case PanelBodyRegion::Content:
        break;
    case PanelBodyRegion::Footer:
        Chrome::PaintDockFooterDivider(context, geometry);
        break;
    case PanelBodyRegion::Count:
        break;
    }
}

bool RegionUsesHorizontalInset(PanelBodyRegion region) {
    return region == PanelBodyRegion::Search || region == PanelBodyRegion::Toolbar;
}

Rect InsetRegionContent(const Rect& regionGeometry, PanelBodyRegion region) {
    if (!RegionUsesHorizontalInset(region)) {
        return regionGeometry;
    }
    const float padH = Chrome::PanelPaddingH();
    return Rect{
        regionGeometry.x + padH,
        regionGeometry.y,
        std::max(0.0f, regionGeometry.width - padH * 2.0f),
        regionGeometry.height
    };
}

} // namespace

PanelBodyLayout::PanelBodyLayout() {
    SetFlexGrow(1.0f);
    SetFlexShrink(1.0f);
}

void PanelBodyLayout::ClearRegions() {
    ClearChildren();
    for (auto& slot : m_Regions) {
        slot.widget.reset();
        slot.geometry = {};
    }
    m_ContentClipRect = {};
}

void PanelBodyLayout::SetRegion(const PanelBodyRegion region, const std::shared_ptr<Widget>& widget) {
    const size_t index = RegionIndex(region);
    if (m_Regions[index].widget == widget) {
        return;
    }
    if (m_Regions[index].widget) {
        RemoveChild(m_Regions[index].widget);
    }
    m_Regions[index].widget = widget;
    m_Regions[index].geometry = {};
    if (widget) {
        if (region == PanelBodyRegion::Content) {
            widget->SetFlexGrow(1.0f);
            widget->SetFlexShrink(1.0f);
        } else {
            widget->SetFlexShrink(0.0f);
        }
        AddChild(widget);
    }
    InvalidateLayout();
}

std::shared_ptr<Widget> PanelBodyLayout::GetRegion(const PanelBodyRegion region) const {
    return m_Regions[RegionIndex(region)].widget;
}

Rect PanelBodyLayout::GetRegionRect(const PanelBodyRegion region) const {
    return m_Regions[RegionIndex(region)].geometry;
}

float PanelBodyLayout::IntrinsicRegionHeight(const PanelBodyRegion region) const {
    switch (region) {
    case PanelBodyRegion::ModeTabs:
        return Chrome::ModeTabRowHeight();
    case PanelBodyRegion::Search:
        return Chrome::SearchRowHeight();
    case PanelBodyRegion::Toolbar:
        return m_OverlayToolbar ? Chrome::ViewportToolbarRowHeight() : Chrome::ToolbarRowHeight();
    case PanelBodyRegion::ColumnHeader:
        return Chrome::ColumnHeaderRowHeight();
    case PanelBodyRegion::Footer:
        return Chrome::FooterRowHeight();
    case PanelBodyRegion::Content:
    case PanelBodyRegion::Count:
        return 0.0f;
    }
    return 0.0f;
}

Size PanelBodyLayout::Measure(const Size& availableSize) {
    float reservedHeight = 0.0f;
    float maxWidth = 0.0f;

    auto measureFixed = [&](PanelBodyRegion region) {
        const auto& slot = m_Regions[RegionIndex(region)];
        if (!slot.widget || !slot.widget->IsVisible()) {
            return;
        }
        const float intrinsicH = IntrinsicRegionHeight(region);
        Size childAvail = availableSize;
        if (childAvail.height < 1.0e8f) {
            childAvail.height = intrinsicH;
        }
        const Size desired = slot.widget->Measure(childAvail);
        const float measuredH = std::max(intrinsicH, desired.height);
        reservedHeight += measuredH;
        maxWidth = std::max(maxWidth, desired.width);
    };

    measureFixed(PanelBodyRegion::ModeTabs);
    if (!m_OverlayToolbar) {
        measureFixed(PanelBodyRegion::Toolbar);
    }
    measureFixed(PanelBodyRegion::Search);
    measureFixed(PanelBodyRegion::ColumnHeader);
    measureFixed(PanelBodyRegion::Footer);

    int regionGapCount = 0;
    auto countGap = [&](PanelBodyRegion region) {
        const auto& slot = m_Regions[RegionIndex(region)];
        if (slot.widget && slot.widget->IsVisible()) {
            ++regionGapCount;
        }
    };
    countGap(PanelBodyRegion::ModeTabs);
    if (!m_OverlayToolbar) {
        countGap(PanelBodyRegion::Toolbar);
    }
    countGap(PanelBodyRegion::Search);
    countGap(PanelBodyRegion::ColumnHeader);
    countGap(PanelBodyRegion::Footer);
    reservedHeight += static_cast<float>(regionGapCount) * RegionSeparationGap();

    Size rowAvailable = availableSize;
    if (rowAvailable.height < 1.0e8f) {
        rowAvailable.height = std::max(0.0f, rowAvailable.height - reservedHeight);
    }

    Size contentDesired{ 0.0f, 0.0f };
    if (m_Regions[RegionIndex(PanelBodyRegion::Content)].widget) {
        contentDesired = m_Regions[RegionIndex(PanelBodyRegion::Content)].widget->Measure(rowAvailable);
        maxWidth = std::max(maxWidth, contentDesired.width);
    }

    const float desiredW = (availableSize.width < 1.0e8f)
        ? availableSize.width
        : maxWidth;
    const float desiredH = (availableSize.height < 1.0e8f)
        ? availableSize.height
        : (reservedHeight + contentDesired.height);

    m_DesiredSize = ClampDesiredSize(Size{ desiredW, desiredH });
    return m_DesiredSize;
}

void PanelBodyLayout::ArrangeFixedRegion(
    PanelBodyRegion region,
    float& currentY,
    const Rect& allottedRect) {
    auto& slot = m_Regions[RegionIndex(region)];
    const float totalBottom = allottedRect.y + allottedRect.height;

    if (!slot.widget || !slot.widget->IsVisible()) {
        slot.geometry = {};
        return;
    }

    const float intrinsicH = IntrinsicRegionHeight(region);
    const float availableH = std::max(0.0f, totalBottom - currentY);

    float regionH = intrinsicH;
    if (regionH > 0.0f) {
        slot.widget->Measure(Size{ allottedRect.width, regionH });
        regionH = std::min(availableH, regionH);
    } else {
        slot.widget->Measure(Size{ allottedRect.width, availableH });
        regionH = std::min(availableH, std::max(0.0f, slot.widget->GetDesiredSize().height));
    }

    slot.geometry = ClampRectToParent(
        Rect{ allottedRect.x, currentY, allottedRect.width, regionH },
        allottedRect);
    AssertLayoutRectValid("PanelBodyLayout.region", slot.geometry, allottedRect);
    slot.widget->Arrange(InsetRegionContent(slot.geometry, region));
    currentY += slot.geometry.height;
    if (slot.geometry.height > 0.01f) {
        currentY = std::min(currentY + RegionSeparationGap(), totalBottom);
    }
}

void PanelBodyLayout::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    ClearLayoutDirty();

    float currentY = allottedRect.y;
    const float totalBottom = allottedRect.y + allottedRect.height;

    ArrangeFixedRegion(PanelBodyRegion::ModeTabs, currentY, allottedRect);
    if (!m_OverlayToolbar) {
        ArrangeFixedRegion(PanelBodyRegion::Toolbar, currentY, allottedRect);
    }
    ArrangeFixedRegion(PanelBodyRegion::Search, currentY, allottedRect);
    ArrangeFixedRegion(PanelBodyRegion::ColumnHeader, currentY, allottedRect);

    float footerHeight = 0.0f;
    auto& footerSlot = m_Regions[RegionIndex(PanelBodyRegion::Footer)];
    if (footerSlot.widget && footerSlot.widget->IsVisible()) {
        const float intrinsicH = IntrinsicRegionHeight(PanelBodyRegion::Footer);
        const float availableH = std::max(0.0f, totalBottom - currentY);
        footerSlot.widget->Measure(Size{ allottedRect.width, availableH });
        footerHeight = std::min(
            std::max(intrinsicH, footerSlot.widget->GetDesiredSize().height),
            availableH);
    }

    float footerGap = 0.0f;
    if (footerSlot.widget && footerSlot.widget->IsVisible() && footerHeight > 0.0f) {
        footerGap = RegionSeparationGap();
    }

    const float contentHeight = std::max(0.0f, totalBottom - currentY - footerHeight - footerGap);
    auto& contentSlot = m_Regions[RegionIndex(PanelBodyRegion::Content)];
    if (contentSlot.widget) {
        contentSlot.geometry = ClampRectToParent(
            Rect{ allottedRect.x, currentY, allottedRect.width, contentHeight },
            allottedRect);
        AssertLayoutRectValid("PanelBodyLayout.content", contentSlot.geometry, allottedRect);
        contentSlot.widget->Arrange(contentSlot.geometry);
        m_ContentClipRect = contentSlot.geometry;
        currentY += contentSlot.geometry.height;

        auto& toolbarSlot = m_Regions[RegionIndex(PanelBodyRegion::Toolbar)];
        if (m_OverlayToolbar && toolbarSlot.widget && toolbarSlot.widget->IsVisible()) {
            const float toolbarH = IntrinsicRegionHeight(PanelBodyRegion::Toolbar);
            const float inset = Chrome::PanelPaddingH();
            toolbarSlot.geometry = ClampRectToParent(
                Rect{
                    contentSlot.geometry.x + inset,
                    contentSlot.geometry.y + inset,
                    std::max(0.0f, contentSlot.geometry.width - inset * 2.0f),
                    std::min(toolbarH, std::max(0.0f, contentSlot.geometry.height - inset))
                },
                allottedRect);
            toolbarSlot.widget->Arrange(toolbarSlot.geometry);
        } else if (m_OverlayToolbar && toolbarSlot.widget) {
            toolbarSlot.geometry = {};
        }
    } else {
        contentSlot.geometry = {};
        m_ContentClipRect = {};
    }

    if (footerSlot.widget && footerSlot.widget->IsVisible() && footerHeight > 0.0f) {
        currentY = std::min(currentY + RegionSeparationGap(), totalBottom);
        footerSlot.geometry = ClampRectToParent(
            Rect{ allottedRect.x, currentY, allottedRect.width, footerHeight },
            allottedRect);
        AssertLayoutRectValid("PanelBodyLayout.region", footerSlot.geometry, allottedRect);
        footerSlot.widget->Arrange(footerSlot.geometry);
    } else if (footerSlot.widget) {
        footerSlot.geometry = {};
    }
}

void PanelBodyLayout::Paint(PaintContext& context) {
    if (!m_Geometry.IsEmpty()) {
        Chrome::PaintNavigationRegion(context, m_Geometry);
    }

    const auto paintRegion = [&](PanelBodyRegion region, bool paintAfterContent) {
        const size_t index = RegionIndex(region);
        const auto& slot = m_Regions[index];
        if (!slot.widget || slot.geometry.IsEmpty()) {
            return;
        }

        const bool skipRegionChrome =
            (m_SuppressContentSurfaces && region == PanelBodyRegion::Content)
            || (m_OverlayToolbar && region == PanelBodyRegion::Toolbar);
        if (!skipRegionChrome) {
            PaintRegionBackground(region, context, slot.geometry);
        }

        if (region == PanelBodyRegion::Content) {
            context.PushClipRect(m_ContentClipRect);
            slot.widget->Paint(context);
            context.PopClipRect();
        } else {
            slot.widget->Paint(context);
        }

        if (!skipRegionChrome) {
            PaintRegionChrome(region, context, slot.geometry);
        }
        (void)paintAfterContent;
    };

    for (size_t i = 0; i < m_Regions.size(); ++i) {
        const auto region = static_cast<PanelBodyRegion>(i);
        if (m_OverlayToolbar && region == PanelBodyRegion::Toolbar) {
            continue;
        }
        paintRegion(region, false);
    }

    if (m_OverlayToolbar) {
        paintRegion(PanelBodyRegion::Toolbar, true);
    }
}

void PanelBodyLayout::RoutePointer(
    const MouseEvent& event,
    void (Widget::*handler)(const MouseEvent&)) {
    for (size_t i = 0; i < m_Regions.size(); ++i) {
        const auto& slot = m_Regions[i];
        if (!slot.widget || !slot.geometry.Contains(event.position)) {
            continue;
        }
        (slot.widget.get()->*handler)(event);
        return;
    }
}

void PanelBodyLayout::OnMouseDown(const MouseEvent& event) {
    RoutePointer(event, &Widget::OnMouseDown);
}

void PanelBodyLayout::OnMouseMove(const MouseEvent& event) {
    RoutePointer(event, &Widget::OnMouseMove);
}

void PanelBodyLayout::OnMouseUp(const MouseEvent& event) {
    RoutePointer(event, &Widget::OnMouseUp);
}

void PanelBodyLayout::OnMouseWheel(const MouseEvent& event) {
    const auto& contentSlot = m_Regions[RegionIndex(PanelBodyRegion::Content)];
    if (contentSlot.widget && contentSlot.geometry.Contains(event.position)) {
        contentSlot.widget->OnMouseWheel(event);
        return;
    }
    RoutePointer(event, &Widget::OnMouseWheel);
}

std::shared_ptr<Widget> PanelBodyLayout::HitTestPoint(const Point& pos, const Rect* clip) {
    if (!IsVisible() || IsPointerTransparent() || !IsEnabled()) {
        return nullptr;
    }
    if ((clip != nullptr && !clip->Contains(pos)) || !m_Geometry.Contains(pos)) {
        return nullptr;
    }

    for (auto it = m_Regions.rbegin(); it != m_Regions.rend(); ++it) {
        const auto& slot = *it;
        if (!slot.widget || slot.geometry.IsEmpty() || !slot.geometry.Contains(pos)) {
            continue;
        }

        Rect regionClip = slot.geometry;
        if (clip != nullptr) {
            regionClip = regionClip.Intersect(*clip);
        }
        if (regionClip.IsEmpty()) {
            continue;
        }

        if (auto hit = slot.widget->HitTestPoint(pos, &regionClip)) {
            return hit;
        }
    }

    return shared_from_this();
}

} // namespace we::editor::panels
