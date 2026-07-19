#include "PropertyEditor/IPropertyEditorRuntime.h"
#include "PropertyEditorInternal.h"

#include "Reflection/IPropertyAccessor.h"
#include "Reflection/Reflection.h"

#include <unordered_map>

namespace we::editor::property {
namespace detail {

class PropertyEditorRuntimeImpl final : public IPropertyEditorRuntime {
public:
    explicit PropertyEditorRuntimeImpl(PropertyEditorDependencies deps)
        : m_Deps(std::move(deps))
    {
        if (m_Deps.typeRegistry) {
            m_Registry = m_Deps.typeRegistry;
        } else {
            m_Registry = &reflection::GetTypeRegistry();
        }

        if (m_Deps.propertyAccessor) {
            m_Accessor = m_Deps.propertyAccessor;
        } else {
            reflection::PropertyAccessorDependencies accessorDeps;
            accessorDeps.registry = m_Registry;
            m_OwnedAccessor = reflection::CreatePropertyAccessor(accessorDeps);
            m_Accessor = m_OwnedAccessor.get();
        }

        m_Factory = CreateEditorFactory(m_Registry, m_Deps.registerDefaultEditors);
        if (m_Deps.onLog) {
            m_Deps.onLog("PropertyEditorRuntime: constructed");
        }
    }

    ~PropertyEditorRuntimeImpl() override { Shutdown(); }

    [[nodiscard]] std::unique_ptr<IDetailsView> MakeDetailsView() override {
        RuntimeServices services;
        services.registry = m_Registry;
        services.accessor = m_Accessor;
        services.serializer = m_Deps.serializer;
        services.transactionHook = m_Deps.transactionHook;
        services.onLog = m_Deps.onLog;
        services.factory = m_Factory.get();
        services.runtime = this;
        return CreateDetailsView(std::move(services));
    }

    [[nodiscard]] IPropertyEditorFactory& EditorFactory() noexcept override { return *m_Factory; }
    [[nodiscard]] reflection::ITypeRegistry& TypeRegistry() noexcept override { return *m_Registry; }
    [[nodiscard]] reflection::IPropertyAccessor& Accessor() noexcept override { return *m_Accessor; }

    void RegisterDetailCustomization(TypeId typeId, DetailCustomizationFactory factory) override {
        m_DetailCustomizations[typeId] = std::move(factory);
    }

    void RegisterPropertyCustomization(TypeId propertyTypeId, PropertyCustomizationFactory factory) override {
        m_PropertyCustomizations[propertyTypeId] = std::move(factory);
    }

    [[nodiscard]] DetailCustomizationFactory FindDetailCustomization(TypeId typeId) const override {
        const auto it = m_DetailCustomizations.find(typeId);
        return it == m_DetailCustomizations.end() ? DetailCustomizationFactory{} : it->second;
    }

    [[nodiscard]] PropertyCustomizationFactory FindPropertyCustomization(TypeId propertyTypeId) const override {
        const auto it = m_PropertyCustomizations.find(propertyTypeId);
        return it == m_PropertyCustomizations.end() ? PropertyCustomizationFactory{} : it->second;
    }

    void Shutdown() override {
        m_DetailCustomizations.clear();
        m_PropertyCustomizations.clear();
        m_Factory.reset();
        m_OwnedAccessor.reset();
        m_Accessor = nullptr;
        m_Registry = nullptr;
    }

private:
    PropertyEditorDependencies m_Deps;
    reflection::ITypeRegistry* m_Registry = nullptr;
    reflection::IPropertyAccessor* m_Accessor = nullptr;
    std::unique_ptr<reflection::IPropertyAccessor> m_OwnedAccessor;
    std::unique_ptr<IPropertyEditorFactory> m_Factory;
    std::unordered_map<TypeId, DetailCustomizationFactory> m_DetailCustomizations;
    std::unordered_map<TypeId, PropertyCustomizationFactory> m_PropertyCustomizations;
};

} // namespace detail

std::unique_ptr<IPropertyEditorRuntime> CreatePropertyEditorRuntime(PropertyEditorDependencies deps) {
    return std::make_unique<detail::PropertyEditorRuntimeImpl>(std::move(deps));
}

} // namespace we::editor::property
