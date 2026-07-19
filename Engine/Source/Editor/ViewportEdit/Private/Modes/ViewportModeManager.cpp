#include "ViewportEditInternal.h"

#include <algorithm>
#include <shared_mutex>

namespace we::editor::viewportedit {
namespace detail {
namespace {

class LambdaModeFactory final : public IViewportModeFactory {
public:
    LambdaModeFactory(ViewportModeDescriptor descriptor, ViewportModeFactoryFn factory)
        : m_Descriptor(std::move(descriptor))
        , m_Factory(std::move(factory))
    {}

    [[nodiscard]] ViewportModeDescriptor Describe() const override { return m_Descriptor; }

    [[nodiscard]] std::unique_ptr<IViewportMode> Create() const override {
        return m_Factory ? m_Factory() : nullptr;
    }

private:
    ViewportModeDescriptor m_Descriptor;
    ViewportModeFactoryFn m_Factory;
};

class ViewportModeRegistryImpl final : public IViewportModeRegistry {
public:
    bool RegisterFactory(std::unique_ptr<IViewportModeFactory> factory) override {
        if (!factory) {
            return false;
        }
        const auto desc = factory->Describe();
        if (desc.id.empty()) {
            return false;
        }
        std::unique_lock lock(m_Mutex);
        m_Factories[desc.id] = std::move(factory);
        m_Descriptors[desc.id] = desc;
        return true;
    }

    bool RegisterFactory(ViewportModeDescriptor descriptor, ViewportModeFactoryFn factory) override {
        if (descriptor.id.empty() || !factory) {
            return false;
        }
        return RegisterFactory(
            std::make_unique<LambdaModeFactory>(std::move(descriptor), std::move(factory)));
    }

    bool Unregister(std::string_view modeId) override {
        std::unique_lock lock(m_Mutex);
        const std::string key(modeId);
        m_Factories.erase(key);
        return m_Descriptors.erase(key) > 0;
    }

    [[nodiscard]] bool Contains(std::string_view modeId) const noexcept override {
        std::shared_lock lock(m_Mutex);
        return m_Descriptors.find(std::string(modeId)) != m_Descriptors.end();
    }

    [[nodiscard]] const ViewportModeDescriptor* Find(std::string_view modeId) const noexcept override {
        std::shared_lock lock(m_Mutex);
        auto it = m_Descriptors.find(std::string(modeId));
        return it != m_Descriptors.end() ? &it->second : nullptr;
    }

    [[nodiscard]] std::vector<ViewportModeDescriptor> List() const override {
        std::shared_lock lock(m_Mutex);
        std::vector<ViewportModeDescriptor> out;
        out.reserve(m_Descriptors.size());
        for (const auto& [_, d] : m_Descriptors) {
            out.push_back(d);
        }
        std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
            return a.sortOrder < b.sortOrder || (a.sortOrder == b.sortOrder && a.id < b.id);
        });
        return out;
    }

    [[nodiscard]] std::unique_ptr<IViewportMode> Create(std::string_view modeId) const override {
        std::shared_lock lock(m_Mutex);
        auto it = m_Factories.find(std::string(modeId));
        if (it == m_Factories.end() || !it->second) {
            return nullptr;
        }
        return it->second->Create();
    }

private:
    mutable std::shared_mutex m_Mutex;
    std::unordered_map<std::string, std::unique_ptr<IViewportModeFactory>> m_Factories;
    std::unordered_map<std::string, ViewportModeDescriptor> m_Descriptors;
};

ViewportModeRegistryImpl& RegistrySingleton() {
    static ViewportModeRegistryImpl instance;
    return instance;
}

class SelectMode final : public IViewportMode {
public:
    [[nodiscard]] ViewportModeId GetId() const noexcept override { return ViewportModeId::Select; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return "Select"; }
    void OnEnter(IViewportContext&) override {}
    void OnExit(IViewportContext&) override {}
    void Tick(IViewportContext&, float) override {}
    [[nodiscard]] ViewportToolId PreferredTool() const noexcept override {
        return ViewportToolId::Select;
    }
};

} // namespace

IViewportModeRegistry& GetViewportModeRegistry() noexcept {
    return RegistrySingleton();
}

void RegisterBuiltinModes(IViewportModeRegistry& registry) {
    ViewportModeDescriptor select;
    select.id = "Select";
    select.displayName = "Select";
    select.iconName = "cursor";
    select.builtinId = ViewportModeId::Select;
    select.sortOrder = 0;
    registry.RegisterFactory(std::move(select), []() {
        return std::unique_ptr<IViewportMode>(std::make_unique<SelectMode>());
    });

    // Placeholder descriptors for future modes — factories may be replaced by plugins.
    const struct {
        const char* id;
        const char* name;
        ViewportModeId builtin;
        int order;
    } stubs[] = {
        {"Landscape", "Landscape", ViewportModeId::Landscape, 10},
        {"Foliage", "Foliage", ViewportModeId::Foliage, 20},
        {"MeshPaint", "Mesh Paint", ViewportModeId::MeshPaint, 30},
        {"Modeling", "Modeling", ViewportModeId::Modeling, 40},
        {"Animation", "Animation", ViewportModeId::Animation, 50},
        {"Physics", "Physics", ViewportModeId::Physics, 60},
        {"Navigation", "Navigation", ViewportModeId::Navigation, 70},
        {"Water", "Water", ViewportModeId::Water, 80},
    };

    for (const auto& s : stubs) {
        if (registry.Contains(s.id)) {
            continue;
        }
        ViewportModeDescriptor d;
        d.id = s.id;
        d.displayName = s.name;
        d.builtinId = s.builtin;
        d.sortOrder = s.order;
        const ViewportModeId builtin = s.builtin;
        const std::string display = s.name;
        registry.RegisterFactory(std::move(d), [builtin, display]() {
            class StubMode final : public IViewportMode {
            public:
                StubMode(ViewportModeId id, std::string name)
                    : m_Id(id)
                    , m_Name(std::move(name))
                {}
                [[nodiscard]] ViewportModeId GetId() const noexcept override { return m_Id; }
                [[nodiscard]] std::string_view GetDisplayName() const noexcept override {
                    return m_Name;
                }
                void OnEnter(IViewportContext&) override {}
                void OnExit(IViewportContext&) override {}
                void Tick(IViewportContext&, float) override {}

            private:
                ViewportModeId m_Id;
                std::string m_Name;
            };
            return std::unique_ptr<IViewportMode>(std::make_unique<StubMode>(builtin, display));
        });
    }
}

class ViewportModeManagerImpl final : public IViewportModeManager {
public:
    explicit ViewportModeManagerImpl(IViewportContext& context)
        : m_Context(context)
    {
        RegisterBuiltinModes(GetViewportModeRegistry());
    }

    [[nodiscard]] IViewportModeRegistry& Registry() noexcept override {
        return GetViewportModeRegistry();
    }

    [[nodiscard]] bool LoadMode(std::string_view modeId) override {
        const std::string key(modeId);
        if (m_Loaded.contains(key)) {
            return true;
        }
        auto mode = Registry().Create(modeId);
        if (!mode) {
            return false;
        }
        m_Loaded[key] = std::move(mode);
        ViewportEditDiagnostics::Get().OnModeLoaded();
        return true;
    }

    [[nodiscard]] bool UnloadMode(std::string_view modeId) override {
        const std::string key(modeId);
        if (m_ActiveKey == key) {
            // Fall back to Select before unloading active mode.
            (void)SetActiveMode(std::string_view("Select"));
        }
        if (m_Loaded.erase(key) > 0) {
            ViewportEditDiagnostics::Get().OnModeUnloaded();
            return true;
        }
        return false;
    }

    [[nodiscard]] bool IsLoaded(std::string_view modeId) const noexcept override {
        return m_Loaded.contains(std::string(modeId));
    }

    [[nodiscard]] bool SetActiveMode(std::string_view modeId) override {
        if (!LoadMode(modeId)) {
            return false;
        }
        auto* next = FindLoaded(modeId);
        if (!next) {
            return false;
        }
        if (m_Active && m_ActiveKey == modeId) {
            return true;
        }
        if (m_Active) {
            m_Active->OnExit(m_Context);
        }
        m_Active = next;
        m_ActiveKey = std::string(modeId);
        m_ActiveId = next->GetId();
        m_Active->OnEnter(m_Context);
        ViewportEditDiagnostics::Get().OnModeActivated();
        return true;
    }

    [[nodiscard]] bool SetActiveMode(ViewportModeId modeId) override {
        return SetActiveMode(BuiltinModeName(modeId));
    }

    [[nodiscard]] std::string_view ActiveModeKey() const noexcept override { return m_ActiveKey; }
    [[nodiscard]] ViewportModeId ActiveModeId() const noexcept override { return m_ActiveId; }
    [[nodiscard]] IViewportMode* ActiveMode() noexcept override { return m_Active; }
    [[nodiscard]] const IViewportMode* ActiveMode() const noexcept override { return m_Active; }

    [[nodiscard]] IViewportMode* FindLoaded(std::string_view modeId) noexcept override {
        auto it = m_Loaded.find(std::string(modeId));
        return it != m_Loaded.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] std::vector<std::string> LoadedModes() const override {
        std::vector<std::string> out;
        out.reserve(m_Loaded.size());
        for (const auto& [k, _] : m_Loaded) {
            out.push_back(k);
        }
        return out;
    }

    void RegisterModeInstance(std::unique_ptr<IViewportMode> mode) override {
        if (!mode) {
            return;
        }
        const std::string key(mode->GetStableId());
        m_Loaded[key] = std::move(mode);
    }

private:
    IViewportContext& m_Context;
    std::unordered_map<std::string, std::unique_ptr<IViewportMode>> m_Loaded;
    IViewportMode* m_Active = nullptr;
    std::string m_ActiveKey = "Select";
    ViewportModeId m_ActiveId = ViewportModeId::Select;
};

std::unique_ptr<IViewportModeManager> CreateModeManager(IViewportContext& context) {
    return std::make_unique<ViewportModeManagerImpl>(context);
}

} // namespace detail

IViewportModeRegistry& GetViewportModeRegistry() noexcept {
    return detail::GetViewportModeRegistry();
}

} // namespace we::editor::viewportedit
