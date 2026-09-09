// ==============================================================================
// WindEffects — Undo — IScopedTransaction
// Public API surface for the Undo module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Undo/Export.h"
#include "Undo/UndoTypes.h"

#include <memory>
#include <string_view>

namespace we::editor::undo {

class ITransactionManager;

/// RAII open transaction — End on success path, Cancel on destruction if still open.
class UNDO_API IScopedTransaction {
public:
    virtual ~IScopedTransaction() = default;

    [[nodiscard]] virtual bool IsOpen() const noexcept = 0;
    [[nodiscard]] virtual TransactionId Id() const noexcept = 0;

    virtual void End() = 0;
    virtual void Cancel() = 0;
};

[[nodiscard]] UNDO_API std::unique_ptr<IScopedTransaction> BeginScopedTransaction(
    ITransactionManager& manager,
    std::string_view label,
    TransactionKind kind = TransactionKind::Generic,
    UndoWorldKey worldKey = {});

} // namespace we::editor::undo
