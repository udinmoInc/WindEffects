#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"

#include <span>
#include <vector>

namespace we::editor::viewportedit {

class VIEWPORTEDIT_API IViewportHitTester {
public:
    virtual ~IViewportHitTester() = default;

    /// Screen pixel (viewport-local) → world ray.
    [[nodiscard]] virtual ViewportRay ScreenToRay(float x, float y, float viewportW, float viewportH) const = 0;

    [[nodiscard]] virtual ViewportHit Pick(float x, float y, float viewportW, float viewportH) const = 0;
    [[nodiscard]] virtual std::vector<ViewportHit> PickBox(
        const ViewportRect& rect,
        float viewportW,
        float viewportH) const = 0;

    /// Frustum / marquee selection foundation (box in screen space).
    [[nodiscard]] virtual std::vector<ViewportObjectId> QueryBox(
        const ViewportRect& rect,
        float viewportW,
        float viewportH) const = 0;
};

} // namespace we::editor::viewportedit
