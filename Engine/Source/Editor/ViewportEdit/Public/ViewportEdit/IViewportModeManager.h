#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"
#include "ViewportEdit/IViewportTool.h"

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::editor::viewportedit {

class IViewportContext;
class IViewportMode;
class IViewportTool;
class IViewportOverlay;
class IViewportRenderExtension;
class IViewportInteractionLayer;

/// Creates mode instances. Plugins implement and register factories — core never hardcodes modes.
class VIEWPORTEDIT_API IViewportModeFactory {
public:
    virtual ~IViewportModeFactory() = default;

    [[nodiscard]] virtual ViewportModeDescriptor Describe() const = 0;
    [[nodiscard]] virtual std::unique_ptr<IViewportMode> Create() const = 0;
};

using ViewportModeFactoryFn = std::function<std::unique_ptr<IViewportMode>()>;

/// Global / workspace registry of mode factories (thread-safe registration).
class VIEWPORTEDIT_API IViewportModeRegistry {
public:
    virtual ~IViewportModeRegistry() = default;

    virtual bool RegisterFactory(std::unique_ptr<IViewportModeFactory> factory) = 0;
    virtual bool RegisterFactory(ViewportModeDescriptor descriptor, ViewportModeFactoryFn factory) = 0;
    virtual bool Unregister(std::string_view modeId) = 0;

    [[nodiscard]] virtual bool Contains(std::string_view modeId) const noexcept = 0;
    [[nodiscard]] virtual const ViewportModeDescriptor* Find(std::string_view modeId) const noexcept = 0;
    [[nodiscard]] virtual std::vector<ViewportModeDescriptor> List() const = 0;

    [[nodiscard]] virtual std::unique_ptr<IViewportMode> Create(std::string_view modeId) const = 0;
};

[[nodiscard]] VIEWPORTEDIT_API IViewportModeRegistry& GetViewportModeRegistry() noexcept;

/// Loads / unloads modes without modifying core viewport code.
class VIEWPORTEDIT_API IViewportModeManager {
public:
    virtual ~IViewportModeManager() = default;

    [[nodiscard]] virtual IViewportModeRegistry& Registry() noexcept = 0;

    /// Dynamically instantiate from registry and enter. Returns false if unknown.
    [[nodiscard]] virtual bool LoadMode(std::string_view modeId) = 0;
    [[nodiscard]] virtual bool UnloadMode(std::string_view modeId) = 0;
    [[nodiscard]] virtual bool IsLoaded(std::string_view modeId) const noexcept = 0;

    [[nodiscard]] virtual bool SetActiveMode(std::string_view modeId) = 0;
    [[nodiscard]] virtual bool SetActiveMode(ViewportModeId modeId) = 0;

    [[nodiscard]] virtual std::string_view ActiveModeKey() const noexcept = 0;
    [[nodiscard]] virtual ViewportModeId ActiveModeId() const noexcept = 0;
    [[nodiscard]] virtual IViewportMode* ActiveMode() noexcept = 0;
    [[nodiscard]] virtual const IViewportMode* ActiveMode() const noexcept = 0;

    [[nodiscard]] virtual IViewportMode* FindLoaded(std::string_view modeId) noexcept = 0;
    [[nodiscard]] virtual std::vector<std::string> LoadedModes() const = 0;

    /// Register a pre-built mode instance (tests / host injection).
    virtual void RegisterModeInstance(std::unique_ptr<IViewportMode> mode) = 0;
};

} // namespace we::editor::viewportedit
