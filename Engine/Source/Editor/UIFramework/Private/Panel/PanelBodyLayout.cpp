#include "WindEffects/Editor/UI/Panel/PanelBodyLayout.h"

#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Layout/LayoutAssert.h"
#include "KindUI/Core/LayoutMetrics.h"

#include <algorithm>

namespace we::editor::panels {
namespace Chrome = PanelChrome;
using ::we::runtime::kindui::AssertLayoutRectValid;
using ::we::runtime::kindui::ClampRectToParent;
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

void PaintRegionChrome(PanelBodyRegion region, PaintContext& context, const Rect& geometry) {
    switch (region) {
    case PanelBodyRegion::ModeTabs:
        Chrome::PaintDockTabStripDivider(context, geometry);
        break;
    case PanelBodyRegion::Search:
    case PanelBodyRegion::Toolbar:
        Chrome::PaintToolbarRegion(context, geometry);
        break;
    case PanelBodyRegion::ColumnHeader:
        Chrome::PaintHeaderRegion(context, geometry);
        break;
    case PanelBodyRegion::Content:
        Chrome::PaintContentWell(context, geometry);
        break;
    case PanelBodyRegion::Footer:
        Chrome::PaintFooterRegion(context, geometry);
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
        return Chrome::ToolbarRowHeight();
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
    measureFixed(PanelBodyRegion::Search);
    if (!m_OverlayToolbar) {
        measureFixed(PanelBodyRegion::Toolbar);
    }
    measureFixed(PanelBodyRegion::ColumnHeader);
    measureFixed(PanelBodyRegion::Footer);

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
    slot.widget->Measure(Size{ allottedRect.width, availableH });
    const float measuredH = std::max(intrinsicH, slot.widget->GetDesiredSize().height);

    slot.geometry = ClampRectToParent(
        Rect{ allottedRect.x, currentY, allottedRect.width, std::min(measuredH, availableH) },
        allottedRect);
    AssertLayoutRectValid("PanelBodyLayout.region", slot.geometry, allottedRect);
    slot.widget->Arrange(InsetRegionContent(slot.geometry, region));
    currentY += slot.geometry.height;
}

void PanelBodyLayout::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    ClearLayoutDirty();

    float currentY = allottedRect.y;
    const float totalBottom = allottedRect.y + allottedRect.height;

    ArrangeFixedRegion(PanelBodyRegion::ModeTabs, currentY, allottedRect);
    ArrangeFixedRegion(PanelBodyRegion::Search, currentY, allottedRect);
    if (!m_OverlayToolbar) {
        ArrangeFixedRegion(PanelBodyRegion::Toolbar, currentY, allottedRect);
    }
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

    const float contentHeight = std::max(0.0f, totalBottom - currentY - footerHeight);
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
            toolbarSlot.geometry = ClampRectToParent(
                Rect{
                    contentSlot.geometry.x,
                    contentSlot.geometry.y,
                    contentSlot.geometry.width,
                    std::min(toolbarH, contentSlot.geometry.height)
                },
                allottedRect);
            toolbarSlot.widget->Arrange(InsetRegionContent(toolbarSlot.geometry, PanelBodyRegion::Toolbar));
        } else if (m_OverlayToolbar && toolbarSlot.widget) {
            toolbarSlot.geometry = {};
        }
    } else {
        contentSlot.geometry = {};
        m_ContentClipRect = {};
    }

    if (footerSlot.widget && footerSlot.widget->IsVisible() && footerHeight > 0.0f) {
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
    if (!m_SuppressContentSurfaces) {
        Chrome::PaintPanelSurface(context, m_Geometry);
    }

    const auto paintRegion = [&](PanelBodyRegion region, bool paintAfterContent) {
        const size_t index = RegionIndex(region);
        const auto& slot = m_Regions[index];
        if (!slot.widget || slot.geometry.IsEmpty()) {
            return;
        }

        const bool skipRegionChrome =
            (m_SuppressContentSurfaces
                && (region == PanelBodyRegion::Content || region == PanelBodyRegion::Footer))
            || (m_OverlayToolbar && region == PanelBodyRegion::Toolbar);
        if (!skipRegionChrome) {
            PaintRegionChrome(region, context, slot.geometry);
        }

        if (region == PanelBodyRegion::Content) {
            context.PushClipRect(m_ContentClipRect);
            slot.widget->Paint(context);
            context.PopClipRect();
        } else {
            slot.widget->Paint(context);
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
