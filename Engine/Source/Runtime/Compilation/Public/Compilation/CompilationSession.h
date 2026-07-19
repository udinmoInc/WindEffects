#pragma once

#include "Compilation/Export.h"
#include "Compilation/ICompilationRuntime.h"

#include <memory>

namespace we::runtime::compilation {

/// Process/session binding for Editor, CLI, and IgniteBT hosts that cannot take constructor DI.
class COMPILATION_API CompilationSession {
public:
    static void Install(std::shared_ptr<ICompilationRuntime> runtime);
    static void Clear() noexcept;

    [[nodiscard]] static ICompilationRuntime* Runtime() noexcept;
    [[nodiscard]] static ICompilationManager* Manager() noexcept;
    [[nodiscard]] static std::shared_ptr<ICompilationRuntime> RuntimeShared() noexcept;
    [[nodiscard]] static bool IsInstalled() noexcept;
};

} // namespace we::runtime::compilation
