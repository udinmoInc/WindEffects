#include "PropertyEditorInternal.h"
#include "PropertyEditor/IPropertyHandle.h"
#include "PropertyEditor/IPropertyNode.h"
#include "PropertyEditor/IPropertyTree.h"
#include "PropertyEditor/MultiObjectEdit.h"

#include <cctype>

namespace we::editor::property {
namespace detail {

class PropertyHandleImpl final : public IPropertyHandle,
                                 public std::enable_shared_from_this<PropertyHandleImpl> {
public:
    PropertyHandleImpl(
        RuntimeServices services,
        TypeId ownerTypeId,
        std::string path,
        const PropertyInfo* property,
        std::vector<void*> instances,
        PropertyValueState valueState)
        : m_Services(std::move(services))
        , m_OwnerTypeId(ownerTypeId)
        , m_Path(std::move(path))
        , m_Property(property)
        , m_Instances(std::move(instances))
        , m_ValueState(valueState)
    {}

    [[nodiscard]] std::string_view GetPath() const noexcept override { return m_Path; }
    [[nodiscard]] TypeId GetOwnerTypeId() const noexcept override { return m_OwnerTypeId; }
    [[nodiscard]] const PropertyInfo* GetPropertyInfo() const noexcept override { return m_Property; }
    [[nodiscard]] PropertyValueState GetValueState() const noexcept override { return m_ValueState; }
    [[nodiscard]] bool IsValid() const noexcept override {
        return m_Property != nullptr && !m_Instances.empty() && m_Services.accessor != nullptr;
    }
    [[nodiscard]] bool IsReadOnly() const noexcept override {
        return m_Property && HasFlag(m_Property->flags, PropertyFlags::ReadOnly);
    }
    [[nodiscard]] const std::vector<void*>& GetInstances() const noexcept override { return m_Instances; }

    [[nodiscard]] bool GetRaw(void* outValue, std::size_t outSize) const override {
        if (!IsValid() || !outValue || m_ValueState == PropertyValueState::Mixed) {
            return false;
        }
        return reflection::GetPropertyPathRaw(
            *m_Services.registry, m_OwnerTypeId, m_Instances.front(), m_Path, outValue, outSize);
    }

    [[nodiscard]] bool SetRaw(const void* value, std::size_t valueSize) override {
        if (!IsValid() || !value || IsReadOnly()) {
            return false;
        }
        return CommitEdit(value, valueSize);
    }

    [[nodiscard]] bool GetBool(bool& out) const override {
        return GetRaw(&out, sizeof(out));
    }
    [[nodiscard]] bool SetBool(bool value) override { return SetRaw(&value, sizeof(value)); }
    [[nodiscard]] bool GetInt32(std::int32_t& out) const override { return GetRaw(&out, sizeof(out)); }
    [[nodiscard]] bool SetInt32(std::int32_t value) override { return SetRaw(&value, sizeof(value)); }
    [[nodiscard]] bool GetUInt32(std::uint32_t& out) const override { return GetRaw(&out, sizeof(out)); }
    [[nodiscard]] bool SetUInt32(std::uint32_t value) override { return SetRaw(&value, sizeof(value)); }
    [[nodiscard]] bool GetInt64(std::int64_t& out) const override { return GetRaw(&out, sizeof(out)); }
    [[nodiscard]] bool SetInt64(std::int64_t value) override { return SetRaw(&value, sizeof(value)); }
    [[nodiscard]] bool GetFloat(float& out) const override { return GetRaw(&out, sizeof(out)); }
    [[nodiscard]] bool SetFloat(float value) override { return SetRaw(&value, sizeof(value)); }
    [[nodiscard]] bool GetDouble(double& out) const override { return GetRaw(&out, sizeof(out)); }
    [[nodiscard]] bool SetDouble(double value) override { return SetRaw(&value, sizeof(value)); }

    [[nodiscard]] bool GetString(std::string& out) const override {
        if (!IsValid() || m_ValueState == PropertyValueState::Mixed) {
            return false;
        }
        return m_Services.accessor->GetString(m_OwnerTypeId, m_Instances.front(), LeafName(), out);
    }

    [[nodiscard]] bool SetString(std::string_view value) override {
        if (!IsValid() || IsReadOnly()) {
            return false;
        }
        // Path-aware string set: only leaf name for direct properties; nested via raw bytes of std::string
        // is unsafe — prefer accessor for top-level, path raw for POD.
        if (m_Path.find('.') == std::string::npos) {
            PropertyChangeEvent event = MakeEvent();
            NotifyPre(event);
            if (event.cancelled) {
                return false;
            }
            serialization::Snapshot snapshot{};
            const bool haveSnap = CaptureOptionalSnapshot(snapshot);
            bool ok = true;
            for (void* instance : m_Instances) {
                ok = m_Services.accessor->SetString(m_OwnerTypeId, instance, LeafName(), value) && ok;
            }
            if (ok) {
                FinishCommit(event, haveSnap ? &snapshot : nullptr);
            }
            return ok;
        }
        return false;
    }

    void AddChangeListener(PropertyChangeListener listener) override {
        m_Listeners.push_back(std::move(listener));
    }

    void SetValueState(PropertyValueState state) { m_ValueState = state; }

private:
    [[nodiscard]] std::string LeafName() const {
        const auto pos = m_Path.rfind('.');
        return pos == std::string::npos ? m_Path : m_Path.substr(pos + 1);
    }

    PropertyChangeEvent MakeEvent() const {
        PropertyChangeEvent event;
        event.path = m_Path;
        event.typeId = m_OwnerTypeId;
        event.instances = m_Instances;
        event.property = m_Property;
        event.isMultiObject = m_Instances.size() > 1;
        if (m_Property && m_Property->size > 0 && !m_Instances.empty()) {
            event.beforeBytes.resize(m_Property->size);
            (void)reflection::GetPropertyPathRaw(
                *m_Services.registry,
                m_OwnerTypeId,
                m_Instances.front(),
                m_Path,
                event.beforeBytes.data(),
                event.beforeBytes.size());
        }
        return event;
    }

    void NotifyPre(PropertyChangeEvent& event) {
        if (m_Services.transactionHook) {
            m_Services.transactionHook->OnPreEdit(event);
        }
        for (const auto& listener : m_Listeners) {
            if (listener) {
                listener(event);
            }
        }
    }

    [[nodiscard]] bool CaptureOptionalSnapshot(serialization::Snapshot& out) const {
        if (!m_Services.serializer || m_Instances.empty()) {
            return false;
        }
        out = serialization::CaptureSnapshot(*m_Services.serializer, m_OwnerTypeId, m_Instances.front());
        return !out.bytes.empty();
    }

    void FinishCommit(PropertyChangeEvent& event, const serialization::Snapshot* snapshot) {
        if (m_Property && m_Property->size > 0 && !m_Instances.empty()) {
            event.afterBytes.resize(m_Property->size);
            (void)reflection::GetPropertyPathRaw(
                *m_Services.registry,
                m_OwnerTypeId,
                m_Instances.front(),
                m_Path,
                event.afterBytes.data(),
                event.afterBytes.size());
        }

        serialization::BinaryDiff diff{};
        const serialization::BinaryDiff* diffPtr = nullptr;
        if (m_Services.registry && m_Instances.size() == 1 && !event.beforeBytes.empty() &&
            !event.afterBytes.empty()) {
            // Diff via temporary buffers is approximate; use DiffInstances when we have before clone.
            // For hook wiring we still notify with null diff if we cannot reconstruct.
            (void)diff;
        }

        if (m_Services.transactionHook) {
            m_Services.transactionHook->OnCommit(event, diffPtr, snapshot);
            m_Services.transactionHook->OnPostEdit(event);
        }
        for (const auto& listener : m_Listeners) {
            if (listener) {
                listener(event);
            }
        }
        m_ValueState = PropertyValueState::Identical;
    }

    bool CommitEdit(const void* value, std::size_t valueSize) {
        PropertyChangeEvent event = MakeEvent();
        NotifyPre(event);
        if (event.cancelled) {
            return false;
        }

        serialization::Snapshot snapshot{};
        const bool haveSnap = CaptureOptionalSnapshot(snapshot);

        bool ok = true;
        for (void* instance : m_Instances) {
            ok = reflection::SetPropertyPathRaw(
                     *m_Services.registry, m_OwnerTypeId, instance, m_Path, value, valueSize) &&
                 ok;
        }
        if (ok) {
            FinishCommit(event, haveSnap ? &snapshot : nullptr);
        }
        return ok;
    }

    RuntimeServices m_Services;
    TypeId m_OwnerTypeId = reflection::kInvalidTypeId;
    std::string m_Path;
    const PropertyInfo* m_Property = nullptr;
    std::vector<void*> m_Instances;
    PropertyValueState m_ValueState = PropertyValueState::Unavailable;
    std::vector<PropertyChangeListener> m_Listeners;
};

class PropertyNodeImpl final : public IPropertyNode {
public:
    std::string path;
    std::string displayName;
    std::string category;
    const PropertyInfo* property = nullptr;
    int depth = 0;
    int arrayIndex = -1;
    PropertyValueState valueState = PropertyValueState::Unavailable;
    bool categoryNode = false;
    bool expanded = true;
    std::vector<PropertyNodePtr> children;
    PropertyHandlePtr handle;

    [[nodiscard]] std::string_view GetPath() const noexcept override { return path; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return displayName; }
    [[nodiscard]] std::string_view GetCategory() const noexcept override { return category; }
    [[nodiscard]] const PropertyInfo* GetPropertyInfo() const noexcept override { return property; }
    [[nodiscard]] int GetDepth() const noexcept override { return depth; }
    [[nodiscard]] int GetArrayIndex() const noexcept override { return arrayIndex; }
    [[nodiscard]] PropertyValueState GetValueState() const noexcept override { return valueState; }
    [[nodiscard]] bool IsCategoryNode() const noexcept override { return categoryNode; }
    [[nodiscard]] bool IsExpanded() const noexcept override { return expanded; }
    void SetExpanded(bool value) override { expanded = value; }
    [[nodiscard]] const std::vector<PropertyNodePtr>& GetChildren() const noexcept override { return children; }
    [[nodiscard]] PropertyHandlePtr GetHandle() const override { return handle; }
};

class PropertyTreeImpl final : public IPropertyTree {
public:
    explicit PropertyTreeImpl(RuntimeServices services) : m_Services(std::move(services)) {}

    void Build(TypeId typeId, void* instance) override {
        std::vector<void*> instances;
        if (instance) {
            instances.push_back(instance);
        }
        Build(typeId, instances);
    }

    void Build(TypeId typeId, const std::vector<void*>& instances) override {
        m_Bindings.clear();
        if (typeId != reflection::kInvalidTypeId && !instances.empty()) {
            ObjectBinding binding;
            binding.typeId = typeId;
            // Multi-object same type: store all instances on one binding via first + parallel list
            m_TypeId = typeId;
            m_Instances = instances;
            for (void* instance : instances) {
                ObjectBinding b;
                b.typeId = typeId;
                b.instance = instance;
                m_Bindings.push_back(b);
            }
            // Same-type multi-object uses shared instance list on handles — collapse to one logical binding
            if (instances.size() > 1) {
                m_Bindings.clear();
                ObjectBinding b;
                b.typeId = typeId;
                b.instance = instances.front();
                m_Bindings.push_back(b);
            }
        } else {
            m_TypeId = reflection::kInvalidTypeId;
            m_Instances.clear();
        }
        Rebuild();
    }

    void BuildBindings(const std::vector<ObjectBinding>& bindings) override {
        m_Bindings = bindings;
        m_TypeId = bindings.empty() ? reflection::kInvalidTypeId : bindings.front().typeId;
        m_Instances.clear();
        for (const auto& b : m_Bindings) {
            if (b.instance) {
                m_Instances.push_back(b.instance);
            }
        }
        Rebuild();
    }

    void Clear() override {
        m_TypeId = reflection::kInvalidTypeId;
        m_Instances.clear();
        m_Bindings.clear();
        m_Roots.clear();
        m_FilteredRoots.clear();
        m_PathIndex.clear();
    }

    void Rebuild() override {
        m_Roots.clear();
        m_PathIndex.clear();
        if (!m_Services.registry || m_Bindings.empty()) {
            ApplyFilter(m_Filter);
            return;
        }

        // Same-type multi-object edit (SetObjects): one binding type, many instances
        if (m_Bindings.size() == 1 && m_Instances.size() > 1 &&
            m_Bindings.front().typeId == m_TypeId) {
            BuildTypeNodes(m_TypeId, m_Instances);
            ApplyFilter(m_Filter);
            return;
        }

        for (const auto& binding : m_Bindings) {
            if (binding.typeId == reflection::kInvalidTypeId || !binding.instance) {
                continue;
            }
            std::vector<void*> instances{binding.instance};
            BuildTypeNodes(binding.typeId, instances);
        }
        ApplyFilter(m_Filter);
    }

    void ApplyFilter(const PropertyFilterOptions& filter) override {
        m_Filter = filter;
        m_FilteredRoots.clear();
        for (const auto& root : m_Roots) {
            auto filtered = FilterNode(root);
            if (filtered) {
                m_FilteredRoots.push_back(filtered);
            }
        }
    }

    [[nodiscard]] const PropertyFilterOptions& GetFilter() const noexcept override { return m_Filter; }
    [[nodiscard]] const std::vector<PropertyNodePtr>& GetRootNodes() const noexcept override { return m_Roots; }
    [[nodiscard]] const std::vector<PropertyNodePtr>& GetFilteredRootNodes() const noexcept override {
        return m_FilteredRoots;
    }

    [[nodiscard]] PropertyNodePtr FindByPath(std::string_view path) const override {
        const auto it = m_PathIndex.find(std::string(path));
        return it == m_PathIndex.end() ? nullptr : it->second;
    }

    [[nodiscard]] TypeId GetTypeId() const noexcept override { return m_TypeId; }
    [[nodiscard]] const std::vector<void*>& GetInstances() const noexcept override { return m_Instances; }

private:
    void BuildTypeNodes(TypeId typeId, const std::vector<void*>& instances) {
        PropertyIterateOptions iterate{};
        iterate.editableOnly = m_Filter.editableOnly;
        iterate.skipHidden = m_Filter.skipHidden;
        iterate.skipTransient = m_Filter.skipTransient;
        iterate.includeBases = true;

        const auto visits = reflection::CollectProperties(*m_Services.registry, typeId, iterate);
        std::unordered_map<std::string, std::shared_ptr<PropertyNodeImpl>> categories;

        for (const auto& visit : visits) {
            if (!visit.property) {
                continue;
            }
            const PropertyInfo& property = *visit.property;
            if (!m_Filter.showAdvanced && HasFlag(property.flags, PropertyFlags::Advanced)) {
                continue;
            }

            const std::string categoryName = CategoryFromProperty(property);
            if (!CategoryAllowed(m_Filter, categoryName)) {
                continue;
            }

            auto catIt = categories.find(categoryName);
            if (catIt == categories.end()) {
                auto cat = std::make_shared<PropertyNodeImpl>();
                cat->displayName = categoryName;
                cat->category = categoryName;
                cat->categoryNode = true;
                cat->depth = 0;
                cat->expanded = true;
                // Reuse existing category root if already present from another binding
                bool found = false;
                for (const auto& root : m_Roots) {
                    if (root && root->IsCategoryNode() && root->GetDisplayName() == categoryName) {
                        cat = std::static_pointer_cast<PropertyNodeImpl>(root);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    m_Roots.push_back(cat);
                }
                categories.emplace(categoryName, cat);
                catIt = categories.find(categoryName);
            }

            auto node = BuildPropertyNode(typeId, instances, property, property.name, 1, categoryName);
            if (node) {
                catIt->second->children.push_back(node);
            }
        }
    }

    PropertyNodePtr FilterNode(const PropertyNodePtr& node) const {
        if (!node) {
            return nullptr;
        }
        auto* impl = static_cast<PropertyNodeImpl*>(node.get());
        if (impl->categoryNode) {
            auto copy = std::make_shared<PropertyNodeImpl>(*impl);
            copy->children.clear();
            for (const auto& child : impl->children) {
                if (auto filtered = FilterNode(child)) {
                    copy->children.push_back(filtered);
                }
            }
            return copy->children.empty() ? nullptr : PropertyNodePtr(copy);
        }

        if (!PassesSearch(impl->displayName, impl->category, m_Filter.searchText)) {
            // Keep if any child matches
            bool anyChild = false;
            for (const auto& child : impl->children) {
                if (FilterNode(child)) {
                    anyChild = true;
                    break;
                }
            }
            if (!anyChild) {
                return nullptr;
            }
        }

        auto copy = std::make_shared<PropertyNodeImpl>(*impl);
        copy->children.clear();
        for (const auto& child : impl->children) {
            if (auto filtered = FilterNode(child)) {
                copy->children.push_back(filtered);
            }
        }
        return copy;
    }

    PropertyValueState ComputeValueState(
        TypeId typeId,
        const std::vector<void*>& instances,
        const std::string& path,
        const PropertyInfo& property) const
    {
        if (instances.empty()) {
            return PropertyValueState::Unavailable;
        }
        if (instances.size() == 1) {
            return PropertyValueState::Identical;
        }
        if (property.size == 0) {
            return PropertyValueState::Unavailable;
        }

        std::vector<std::uint8_t> first(property.size);
        if (!reflection::GetPropertyPathRaw(
                *m_Services.registry, typeId, instances.front(), path, first.data(), first.size())) {
            return PropertyValueState::Unavailable;
        }
        std::vector<std::uint8_t> other(property.size);
        for (std::size_t i = 1; i < instances.size(); ++i) {
            if (!reflection::GetPropertyPathRaw(
                    *m_Services.registry, typeId, instances[i], path, other.data(), other.size())) {
                return PropertyValueState::Unavailable;
            }
            if (std::memcmp(first.data(), other.data(), property.size) != 0) {
                return PropertyValueState::Mixed;
            }
        }
        return PropertyValueState::Identical;
    }

    std::shared_ptr<PropertyNodeImpl> BuildPropertyNode(
        TypeId typeId,
        const std::vector<void*>& instances,
        const PropertyInfo& property,
        const std::string& path,
        int depth,
        const std::string& category)
    {
        auto node = std::make_shared<PropertyNodeImpl>();
        node->path = path;
        node->displayName = property.name;
        node->category = category;
        node->property = &property;
        node->depth = depth;
        node->valueState = ComputeValueState(typeId, instances, path, property);
        node->expanded = depth < 2;

        node->handle = std::make_shared<PropertyHandleImpl>(
            m_Services, typeId, path, &property, instances, node->valueState);

        // Path index: disambiguate across multi-type bindings
        const std::string indexKey = std::to_string(static_cast<unsigned long long>(typeId)) + "|" + path;
        m_PathIndex[indexKey] = node;
        if (m_PathIndex.find(path) == m_PathIndex.end()) {
            m_PathIndex[path] = node;
        }

        if (m_Services.registry) {
            if (const TypeInfo* childType = m_Services.registry->Find(property.typeId)) {
                if (childType->IsClassOrStruct()) {
                    PropertyIterateOptions nestedOpts{};
                    nestedOpts.editableOnly = m_Filter.editableOnly;
                    nestedOpts.skipHidden = m_Filter.skipHidden;
                    nestedOpts.skipTransient = m_Filter.skipTransient;
                    const auto nested = reflection::CollectProperties(
                        *m_Services.registry, property.typeId, nestedOpts);
                    for (const auto& visit : nested) {
                        if (!visit.property) {
                            continue;
                        }
                        if (!m_Filter.showAdvanced &&
                            HasFlag(visit.property->flags, PropertyFlags::Advanced)) {
                            continue;
                        }
                        const std::string childPath = path + "." + visit.property->name;
                        if (auto child = BuildPropertyNode(
                                typeId, instances, *visit.property, childPath, depth + 1, category)) {
                            node->children.push_back(child);
                        }
                    }
                } else if (childType->kind == TypeKind::Array || childType->kind == TypeKind::Map) {
                    node->expanded = false;
                }
            }
        }

        return node;
    }

    RuntimeServices m_Services;
    TypeId m_TypeId = reflection::kInvalidTypeId;
    std::vector<void*> m_Instances;
    std::vector<ObjectBinding> m_Bindings;
    PropertyFilterOptions m_Filter{};
    std::vector<PropertyNodePtr> m_Roots;
    std::vector<PropertyNodePtr> m_FilteredRoots;
    std::unordered_map<std::string, PropertyNodePtr> m_PathIndex;
};

std::shared_ptr<IPropertyTree> CreatePropertyTree(RuntimeServices services) {
    return std::make_shared<PropertyTreeImpl>(std::move(services));
}

} // namespace detail

std::vector<MultiObjectPropertyState> MergeCommonProperties(
    const reflection::ITypeRegistry& registry,
    const MultiObjectBinding& binding,
    const PropertyFilterOptions& filter)
{
    std::vector<MultiObjectPropertyState> result;
    if (binding.typeId == reflection::kInvalidTypeId || binding.instances.empty()) {
        return result;
    }

    reflection::PropertyIterateOptions iterate{};
    iterate.editableOnly = filter.editableOnly;
    iterate.skipHidden = filter.skipHidden;
    iterate.skipTransient = filter.skipTransient;

    const auto visits = reflection::CollectProperties(registry, binding.typeId, iterate);
    for (const auto& visit : visits) {
        if (!visit.property) {
            continue;
        }
        // Avoid naming locals `property` — conflicts with enclosing namespace we::editor::property.
        const reflection::PropertyInfo& propInfo = *visit.property;
        if (!filter.showAdvanced &&
            reflection::HasFlag(propInfo.flags, reflection::PropertyFlags::Advanced)) {
            continue;
        }

        MultiObjectPropertyState state;
        state.path = propInfo.name;
        state.propertySize = propInfo.size;
        if (binding.instances.size() == 1 || propInfo.size == 0) {
            state.valueState = PropertyValueState::Identical;
            if (propInfo.size > 0) {
                state.commonBytes.resize(propInfo.size);
                (void)reflection::GetPropertyPathRaw(
                    registry,
                    binding.typeId,
                    binding.instances.front(),
                    state.path,
                    state.commonBytes.data(),
                    state.commonBytes.size());
            }
        } else {
            state.commonBytes.resize(propInfo.size);
            if (!reflection::GetPropertyPathRaw(
                    registry,
                    binding.typeId,
                    binding.instances.front(),
                    state.path,
                    state.commonBytes.data(),
                    state.commonBytes.size())) {
                state.valueState = PropertyValueState::Unavailable;
            } else {
                state.valueState = PropertyValueState::Identical;
                std::vector<std::uint8_t> other(propInfo.size);
                for (std::size_t i = 1; i < binding.instances.size(); ++i) {
                    if (!reflection::GetPropertyPathRaw(
                            registry,
                            binding.typeId,
                            binding.instances[i],
                            state.path,
                            other.data(),
                            other.size()) ||
                        std::memcmp(state.commonBytes.data(), other.data(), propInfo.size) != 0) {
                        state.valueState = PropertyValueState::Mixed;
                        state.commonBytes.clear();
                        break;
                    }
                }
            }
        }
        result.push_back(std::move(state));
    }
    return result;
}

} // namespace we::editor::property
