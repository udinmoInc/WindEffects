// ==============================================================================
// WindEffects — PropertyEditor — IPropertyTransactionHook
// Public API surface for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/PropertyChangeEvent.h"
#include "Serialization/Delta.h"

namespace we::editor::property {

/// Undo / history integration point. PropertyEditor never owns an undo stack.
class PROPERTYEDITOR_API IPropertyTransactionHook {
public:
    virtual ~IPropertyTransactionHook() = default;

    virtual void OnPreEdit(PropertyChangeEvent& event) { (void)event; }

    /// Called after a successful write. Diff/snapshot are optional (null when serializer absent).
    virtual void OnCommit(
        const PropertyChangeEvent& event,
        const serialization::BinaryDiff* diff,
        const serialization::Snapshot* snapshot)
    {
        (void)event;
        (void)diff;
        (void)snapshot;
    }

    virtual void OnPostEdit(const PropertyChangeEvent& event) { (void)event; }
};

} // namespace we::editor::property
