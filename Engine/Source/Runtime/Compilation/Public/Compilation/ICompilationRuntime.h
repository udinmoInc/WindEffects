#pragma once

#include "Compilation/Export.h"
#include "Compilation/CompilationTypes.h"
#include "Compilation/ICompiler.h"
#include "Compilation/ICompileServices.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::assetruntime {
class IAssetManager;
}

namespace we::runtime::compilation {

class COMPILATION_API ICompilationManager {
public:
    virtual ~ICompilationManager() = default;

    [[nodiscard]] virtual ICompilerRegistry& Registry() noexcept = 0;
    [[nodiscard]] virtual ICompileCache& Cache() noexcept = 0;
    [[nodiscard]] virtual ICompileDatabase& Database() noexcept = 0;
    [[nodiscard]] virtual ICompileDependencyGraph& Dependencies() noexcept = 0;
    [[nodiscard]] virtual ICompileScheduler& Scheduler() noexcept = 0;
    [[nodiscard]] virtual ICompilePipeline& Pipeline() noexcept = 0;
    [[nodiscard]] virtual ICompileNotification& Notifications() noexcept = 0;
    [[nodiscard]] virtual ICompileDiagnostics& Diagnostics() noexcept = 0;
    [[nodiscard]] virtual ICompileProgress& Progress() noexcept = 0;

    [[nodiscard]] virtual std::shared_ptr<ICompileRequest> CreateRequest(
        CompilerKind kind,
        CompileSourceDesc source,
        CompileOptions options = {},
        CompilePriority priority = CompilePriority::Normal) = 0;

    [[nodiscard]] virtual std::shared_ptr<ICompileResult> CompileSync(
        std::shared_ptr<ICompileRequest> request) = 0;

    [[nodiscard]] virtual CompileId CompileAsync(std::shared_ptr<ICompileRequest> request) = 0;

    [[nodiscard]] virtual bool Cancel(CompileId jobId) = 0;
    [[nodiscard]] virtual bool Restart(CompileId jobId) = 0;

    /// Batch: enqueue many, optionally wait for idle.
    [[nodiscard]] virtual std::vector<CompileId> CompileBatch(
        std::vector<std::shared_ptr<ICompileRequest>> requests,
        bool wait = false) = 0;

    /// Invalidate cache + dependents when a source changes (live recompilation hook).
    [[nodiscard]] virtual std::vector<CompileId> InvalidateAndRebuild(std::string_view sourcePath) = 0;

    virtual void WaitForIdle() = 0;
    virtual void Tick(float deltaSeconds) = 0;
};

class COMPILATION_API ICompilationRuntime {
public:
    virtual ~ICompilationRuntime() = default;

    [[nodiscard]] virtual ICompilationManager& Manager() noexcept = 0;
    [[nodiscard]] virtual const ICompilationManager& Manager() const noexcept = 0;

    /// Register stock compilers (HLSL/GLSL/SPIR-V/DXIL/Material/Compute/…).
    virtual void RegisterBuiltinCompilers() = 0;

    virtual void Shutdown() = 0;
};

struct COMPILATION_API CompilationDependencies {
    CompilationConfig config{};
    assetruntime::IAssetManager* assetManager = nullptr; // optional handoff target
    std::function<void(std::string_view)> onLog;
    /// Optional external tool path (e.g. dxc.exe) for real shader backends.
    std::string externalCompilerPath;
};

[[nodiscard]] COMPILATION_API std::unique_ptr<ICompilationRuntime> CreateCompilationRuntime(
    const CompilationDependencies& deps = {});

} // namespace we::runtime::compilation
