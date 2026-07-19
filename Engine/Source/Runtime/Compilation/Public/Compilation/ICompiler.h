#pragma once

#include "Compilation/Export.h"
#include "Compilation/CompilationTypes.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::compilation {

class COMPILATION_API ICompileArtifact {
public:
    virtual ~ICompileArtifact() = default;

    [[nodiscard]] virtual CompileId GetId() const noexcept = 0;
    [[nodiscard]] virtual CompilerKind GetKind() const noexcept = 0;
    [[nodiscard]] virtual CompileTargetFormat GetFormat() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetKey() const noexcept = 0;
    [[nodiscard]] virtual std::span<const std::uint8_t> GetBytes() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t GetContentHash() const noexcept = 0;
    [[nodiscard]] virtual std::uint32_t GetCompilerVersion() const noexcept = 0;
};

class COMPILATION_API ICompileResult {
public:
    virtual ~ICompileResult() = default;

    [[nodiscard]] virtual CompileStatus GetStatus() const noexcept = 0;
    [[nodiscard]] virtual CompileId GetRequestId() const noexcept = 0;
    [[nodiscard]] virtual CompileId GetJobId() const noexcept = 0;
    [[nodiscard]] virtual std::shared_ptr<ICompileArtifact> GetArtifact() const = 0;
    [[nodiscard]] virtual std::span<const CompileDiagnostic> GetDiagnostics() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t GetDurationMicros() const noexcept = 0;
    [[nodiscard]] virtual bool WasCacheHit() const noexcept = 0;
};

class COMPILATION_API ICompileRequest {
public:
    virtual ~ICompileRequest() = default;

    [[nodiscard]] virtual CompileId GetId() const noexcept = 0;
    [[nodiscard]] virtual CompilerKind GetKind() const noexcept = 0;
    [[nodiscard]] virtual CompilePriority GetPriority() const noexcept = 0;
    [[nodiscard]] virtual const CompileSourceDesc& GetSource() const noexcept = 0;
    [[nodiscard]] virtual const CompileOptions& GetOptions() const noexcept = 0;
    [[nodiscard]] virtual std::span<const std::string> GetDependencies() const noexcept = 0;
};

class COMPILATION_API ICompileJob {
public:
    virtual ~ICompileJob() = default;

    [[nodiscard]] virtual CompileId GetId() const noexcept = 0;
    [[nodiscard]] virtual CompileId GetRequestId() const noexcept = 0;
    [[nodiscard]] virtual CompileStatus GetStatus() const noexcept = 0;
    [[nodiscard]] virtual CompilePriority GetPriority() const noexcept = 0;
    virtual void Cancel() = 0;
};

class COMPILATION_API ICompileTask {
public:
    virtual ~ICompileTask() = default;

    [[nodiscard]] virtual CompileId GetJobId() const noexcept = 0;
    [[nodiscard]] virtual bool IsCancelled() const noexcept = 0;
    [[nodiscard]] virtual std::shared_ptr<ICompileResult> Run() = 0;
};

class COMPILATION_API ICompiler {
public:
    virtual ~ICompiler() = default;

    [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
    [[nodiscard]] virtual CompilerKind GetKind() const noexcept = 0;
    [[nodiscard]] virtual std::uint32_t GetVersion() const noexcept = 0;
    [[nodiscard]] virtual bool Supports(CompilerKind kind, CompileTargetFormat format) const noexcept = 0;
    [[nodiscard]] virtual std::vector<CompileTargetFormat> SupportedFormats() const = 0;

    /// Deterministic cache key for the request (source + options + compiler version).
    [[nodiscard]] virtual std::string MakeCacheKey(const ICompileRequest& request) const = 0;

    [[nodiscard]] virtual std::shared_ptr<ICompileResult> Compile(
        const ICompileRequest& request,
        const ICompileTask& task) = 0;
};

class COMPILATION_API ICompilerRegistry {
public:
    virtual ~ICompilerRegistry() = default;

    [[nodiscard]] virtual bool Register(std::shared_ptr<ICompiler> compiler) = 0;
    [[nodiscard]] virtual bool Unregister(CompilerKind kind) = 0;
    [[nodiscard]] virtual std::shared_ptr<ICompiler> Find(CompilerKind kind) const = 0;
    [[nodiscard]] virtual std::shared_ptr<ICompiler> FindByName(std::string_view name) const = 0;
    [[nodiscard]] virtual std::vector<CompilerKind> ListKinds() const = 0;
};

} // namespace we::runtime::compilation
