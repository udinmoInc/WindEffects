#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/PropertyChangeEvent.h"
#include "PropertyEditor/PropertyEditorTypes.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/TypeId.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::editor::property {

/// Typed get/set against one or more bound instances via reflection paths.
class PROPERTYEDITOR_API IPropertyHandle {
public:
    virtual ~IPropertyHandle() = default;

    [[nodiscard]] virtual std::string_view GetPath() const noexcept = 0;
    [[nodiscard]] virtual reflection::TypeId GetOwnerTypeId() const noexcept = 0;
    [[nodiscard]] virtual const reflection::PropertyInfo* GetPropertyInfo() const noexcept = 0;
    [[nodiscard]] virtual PropertyValueState GetValueState() const noexcept = 0;
    [[nodiscard]] virtual bool IsReadOnly() const noexcept = 0;
    [[nodiscard]] virtual bool IsValid() const noexcept = 0;
    [[nodiscard]] virtual const std::vector<void*>& GetInstances() const noexcept = 0;

    [[nodiscard]] virtual bool GetRaw(void* outValue, std::size_t outSize) const = 0;
    [[nodiscard]] virtual bool SetRaw(const void* value, std::size_t valueSize) = 0;

    [[nodiscard]] virtual bool GetBool(bool& out) const = 0;
    [[nodiscard]] virtual bool SetBool(bool value) = 0;
    [[nodiscard]] virtual bool GetInt32(std::int32_t& out) const = 0;
    [[nodiscard]] virtual bool SetInt32(std::int32_t value) = 0;
    [[nodiscard]] virtual bool GetUInt32(std::uint32_t& out) const = 0;
    [[nodiscard]] virtual bool SetUInt32(std::uint32_t value) = 0;
    [[nodiscard]] virtual bool GetInt64(std::int64_t& out) const = 0;
    [[nodiscard]] virtual bool SetInt64(std::int64_t value) = 0;
    [[nodiscard]] virtual bool GetFloat(float& out) const = 0;
    [[nodiscard]] virtual bool SetFloat(float value) = 0;
    [[nodiscard]] virtual bool GetDouble(double& out) const = 0;
    [[nodiscard]] virtual bool SetDouble(double value) = 0;
    [[nodiscard]] virtual bool GetString(std::string& out) const = 0;
    [[nodiscard]] virtual bool SetString(std::string_view value) = 0;

    virtual void AddChangeListener(PropertyChangeListener listener) = 0;
};

using PropertyHandlePtr = std::shared_ptr<IPropertyHandle>;

} // namespace we::editor::property
