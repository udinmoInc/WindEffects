// ==============================================================================
// WindEffects — PropertyEditor — IPropertyEditorRuntime
// Public API surface for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/IDetailCustomization.h"
#include "PropertyEditor/IDetailsView.h"
#include "PropertyEditor/IPropertyEditorFactory.h"
#include "PropertyEditor/PropertyEditorDependencies.h"
#include "Reflection/IPropertyAccessor.h"
#include "Reflection/ITypeRegistry.h"
#include "Reflection/TypeId.h"

#include <memory>

namespace we::editor::property {

/// Top-level PropertyEditor facade — owns factories, customizations, details views.
class PROPERTYEDITOR_API IPropertyEditorRuntime {
public:
    virtual ~IPropertyEditorRuntime() = default;

    [[nodiscard]] virtual std::unique_ptr<IDetailsView> MakeDetailsView() = 0;
    [[nodiscard]] virtual IPropertyEditorFactory& EditorFactory() noexcept = 0;
    [[nodiscard]] virtual reflection::ITypeRegistry& TypeRegistry() noexcept = 0;
    [[nodiscard]] virtual reflection::IPropertyAccessor& Accessor() noexcept = 0;

    virtual void RegisterDetailCustomization(
        reflection::TypeId typeId,
        DetailCustomizationFactory factory) = 0;

    virtual void RegisterPropertyCustomization(
        reflection::TypeId propertyTypeId,
        PropertyCustomizationFactory factory) = 0;

    [[nodiscard]] virtual DetailCustomizationFactory FindDetailCustomization(
        reflection::TypeId typeId) const = 0;

    [[nodiscard]] virtual PropertyCustomizationFactory FindPropertyCustomization(
        reflection::TypeId propertyTypeId) const = 0;

    virtual void Shutdown() = 0;
};

[[nodiscard]] PROPERTYEDITOR_API std::unique_ptr<IPropertyEditorRuntime>
CreatePropertyEditorRuntime(PropertyEditorDependencies deps = {});

} // namespace we::editor::property
