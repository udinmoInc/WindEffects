// ==============================================================================
// WindEffects — Compilation — CompilationSession
// Internal implementation for the Compilation module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Compilation/CompilationSession.h"

namespace we::runtime::compilation {
namespace {

std::shared_ptr<ICompilationRuntime> g_Runtime;

} // namespace

void CompilationSession::Install(std::shared_ptr<ICompilationRuntime> runtime) {
    g_Runtime = std::move(runtime);
}

void CompilationSession::Clear() noexcept {
    g_Runtime.reset();
}

ICompilationRuntime* CompilationSession::Runtime() noexcept {
    return g_Runtime.get();
}

ICompilationManager* CompilationSession::Manager() noexcept {
    return g_Runtime ? &g_Runtime->Manager() : nullptr;
}

std::shared_ptr<ICompilationRuntime> CompilationSession::RuntimeShared() noexcept {
    return g_Runtime;
}

bool CompilationSession::IsInstalled() noexcept {
    return static_cast<bool>(g_Runtime);
}

} // namespace we::runtime::compilation
