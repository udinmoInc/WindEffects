#include "Reflection/IObjectFactory.h"
#include "Reflection/ITypeRegistry.h"

#include <new>

namespace we::runtime::reflection {
namespace {

class ObjectFactory final : public IObjectFactory {
public:
    explicit ObjectFactory(ITypeRegistry* registry) : m_Registry(registry) {}

    [[nodiscard]] void* Create(TypeId typeId) const override {
        if (!m_Registry) {
            return nullptr;
        }
        const TypeInfo* info = m_Registry->Resolve(typeId);
        if (!info) {
            return nullptr;
        }
        if (info->ops.defaultConstruct) {
            return info->ops.defaultConstruct();
        }
        if (info->ops.placementConstruct && info->size > 0) {
            void* memory = ::operator new(info->size);
            info->ops.placementConstruct(memory);
            return memory;
        }
        return nullptr;
    }

    void Destroy(TypeId typeId, void* instance) const override {
        if (!instance || !m_Registry) {
            return;
        }
        const TypeInfo* info = m_Registry->Resolve(typeId);
        if (!info) {
            return;
        }
        if (info->ops.destruct) {
            info->ops.destruct(instance);
            return;
        }
        if (info->ops.placementDestruct) {
            info->ops.placementDestruct(instance);
            ::operator delete(instance);
        }
    }

    [[nodiscard]] bool PlacementNew(TypeId typeId, void* memory) const override {
        if (!memory || !m_Registry) {
            return false;
        }
        const TypeInfo* info = m_Registry->Resolve(typeId);
        if (!info || !info->ops.placementConstruct) {
            return false;
        }
        info->ops.placementConstruct(memory);
        return true;
    }

    void PlacementDelete(TypeId typeId, void* memory) const override {
        if (!memory || !m_Registry) {
            return;
        }
        const TypeInfo* info = m_Registry->Resolve(typeId);
        if (!info || !info->ops.placementDestruct) {
            return;
        }
        info->ops.placementDestruct(memory);
    }

    [[nodiscard]] bool CanCreate(TypeId typeId) const override {
        if (!m_Registry) {
            return false;
        }
        const TypeInfo* info = m_Registry->Resolve(typeId);
        return info && info->ops.CanDefaultConstruct();
    }

private:
    ITypeRegistry* m_Registry = nullptr;
};

} // namespace

std::unique_ptr<IObjectFactory> CreateObjectFactory(ObjectFactoryDependencies deps) {
    return std::make_unique<ObjectFactory>(deps.registry);
}

} // namespace we::runtime::reflection
