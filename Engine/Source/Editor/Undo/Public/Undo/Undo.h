// ==============================================================================
// WindEffects — Undo — Undo
// Public API surface for the Undo module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

// WindEffects Undo/Redo Runtime — umbrella public header.

#include "Undo/Export.h"
#include "Undo/UndoTypes.h"
#include "Undo/UndoDiagnostics.h"
#include "Undo/ITransactionCommand.h"
#include "Undo/ITransaction.h"
#include "Undo/ITransactionHistory.h"
#include "Undo/IMergePolicy.h"
#include "Undo/ITransactionListener.h"
#include "Undo/ITransactionManager.h"
#include "Undo/IScopedTransaction.h"
#include "Undo/IUndoRuntime.h"
#include "Undo/TransactionCommands.h"
#include "Undo/UndoTests.h"
#include "Undo/UndoBenchmark.h"
