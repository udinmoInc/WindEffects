#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"

#include <functional>
#include <span>
#include <vector>

namespace we::editor::viewportedit {

class VIEWPORTEDIT_API IViewportSelection {
public:
    virtual ~IViewportSelection() = default;

    [[nodiscard]] virtual std::span<const ViewportObjectId> GetSelected() const noexcept = 0;
    [[nodiscard]] virtual ViewportObjectId GetPrimary() const noexcept = 0;
    [[nodiscard]] virtual bool IsSelected(ViewportObjectId id) const noexcept = 0;
    [[nodiscard]] virtual bool IsEmpty() const noexcept = 0;
    [[nodiscard]] virtual std::size_t Count() const noexcept = 0;

    virtual void Clear() = 0;
    virtual void Set(ViewportObjectId id) = 0;
    virtual void SetMany(std::span<const ViewportObjectId> ids) = 0;
    virtual void Apply(SelectionOp op, ViewportObjectId id) = 0;
    virtual void ApplyMany(SelectionOp op, std::span<const ViewportObjectId> ids) = 0;

    [[nodiscard]] virtual ViewportObjectId GetHovered() const noexcept = 0;
    virtual void SetHovered(ViewportObjectId id) = 0;

    virtual void SetLocked(ViewportObjectId id, bool locked) = 0;
    virtual void SetHidden(ViewportObjectId id, bool hidden) = 0;
    [[nodiscard]] virtual bool IsLocked(ViewportObjectId id) const noexcept = 0;
    [[nodiscard]] virtual bool IsHidden(ViewportObjectId id) const noexcept = 0;

    using Listener = std::function<void()>;
    virtual void AddListener(Listener listener) = 0;
};

} // namespace we::editor::viewportedit
