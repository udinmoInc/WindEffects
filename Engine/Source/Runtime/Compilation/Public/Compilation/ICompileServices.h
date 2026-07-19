#pragma once

#include "Compilation/Export.h"
#include "Compilation/CompilationTypes.h"
#include "Compilation/ICompiler.h"

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::compilation {

class COMPILATION_API ICompileCache {
public:
    virtual ~ICompileCache() = default;

    [[nodiscard]] virtual std::shared_ptr<ICompileArtifact> TryGet(std::string_view key) const = 0;
    [[nodiscard]] virtual bool Put(std::shared_ptr<ICompileArtifact> artifact) = 0;
    [[nodiscard]] virtual bool Invalidate(std::string_view key) = 0;
    [[nodiscard]] virtual bool InvalidateByDependency(std::string_view dependencyPath) = 0;
    virtual void Clear() = 0;
    [[nodiscard]] virtual std::uint64_t EntryCount() const noexcept = 0;
};

class COMPILATION_API ICompileDatabase {
public:
    virtual ~ICompileDatabase() = default;

    [[nodiscard]] virtual bool Record(const ICompileResult& result) = 0;
    [[nodiscard]] virtual std::shared_ptr<ICompileArtifact> FindArtifact(std::string_view key) const = 0;
    [[nodiscard]] virtual std::vector<std::string> ListKeys() const = 0;
    [[nodiscard]] virtual bool SaveToDisk(std::string_view path) = 0;
    [[nodiscard]] virtual bool LoadFromDisk(std::string_view path) = 0;
};

class COMPILATION_API ICompileDependencyGraph {
public:
    virtual ~ICompileDependencyGraph() = default;

    virtual void AddEdge(std::string_view from, std::string_view to) = 0;
    virtual void RemoveNode(std::string_view node) = 0;
    [[nodiscard]] virtual std::vector<std::string> GetDependents(std::string_view node) const = 0;
    [[nodiscard]] virtual std::vector<std::string> GetDependencies(std::string_view node) const = 0;
    [[nodiscard]] virtual bool HasCycle() const = 0;
    [[nodiscard]] virtual std::vector<std::string> CollectInvalidationSet(std::string_view root) const = 0;
};

class COMPILATION_API ICompileProgress {
public:
    virtual ~ICompileProgress() = default;

    [[nodiscard]] virtual std::uint64_t Queued() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t Running() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t Completed() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t Failed() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t CacheHits() const noexcept = 0;
    [[nodiscard]] virtual float OverallFraction() const noexcept = 0;
};

using CompileNotificationFn = std::function<void(const ICompileResult&)>;

class COMPILATION_API ICompileNotification {
public:
    virtual ~ICompileNotification() = default;
    virtual void Subscribe(CompileNotificationFn listener) = 0;
    virtual void Publish(const ICompileResult& result) = 0;
};

class COMPILATION_API ICompileDiagnostics {
public:
    virtual ~ICompileDiagnostics() = default;

    virtual void Report(const CompileDiagnostic& diagnostic) = 0;
    [[nodiscard]] virtual std::vector<CompileDiagnostic> Snapshot() const = 0;
    virtual void Clear() = 0;
};

class COMPILATION_API ICompileWorker {
public:
    virtual ~ICompileWorker() = default;

    [[nodiscard]] virtual std::uint32_t GetId() const noexcept = 0;
    [[nodiscard]] virtual bool IsBusy() const noexcept = 0;
    virtual void Execute(std::shared_ptr<ICompileTask> task) = 0;
};

class COMPILATION_API ICompileScheduler {
public:
    virtual ~ICompileScheduler() = default;

    virtual void Enqueue(std::shared_ptr<ICompileJob> job) = 0;
    [[nodiscard]] virtual bool Cancel(CompileId jobId) = 0;
    [[nodiscard]] virtual bool Restart(CompileId jobId) = 0;
    virtual void Tick() = 0;
    virtual void Shutdown() = 0;
    [[nodiscard]] virtual ICompileProgress& Progress() noexcept = 0;
};

class COMPILATION_API ICompilePipeline {
public:
    virtual ~ICompilePipeline() = default;

    /// Full path: request → cache lookup → schedule → compile → store → notify.
    [[nodiscard]] virtual std::shared_ptr<ICompileResult> ExecuteSync(
        std::shared_ptr<ICompileRequest> request) = 0;

    [[nodiscard]] virtual CompileId ExecuteAsync(std::shared_ptr<ICompileRequest> request) = 0;
};

} // namespace we::runtime::compilation
