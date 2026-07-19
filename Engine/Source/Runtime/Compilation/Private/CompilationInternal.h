#pragma once

#include "Compilation/Compilation.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <span>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace we::runtime::compilation {
namespace detail {

[[nodiscard]] inline std::uint64_t NowMicros() {
    using clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(clock::now().time_since_epoch()).count());
}

[[nodiscard]] std::uint64_t HashBytes(std::span<const std::uint8_t> bytes) noexcept;
[[nodiscard]] std::uint64_t HashString(std::string_view text) noexcept;
[[nodiscard]] std::string MakeDeterministicKey(
    CompilerKind kind,
    std::uint32_t compilerVersion,
    const CompileSourceDesc& source,
    const CompileOptions& options);

class ArtifactImpl final : public ICompileArtifact {
public:
    CompileId id{};
    CompilerKind kind = CompilerKind::Unknown;
    CompileTargetFormat format = CompileTargetFormat::BinaryBlob;
    std::string key;
    std::vector<std::uint8_t> bytes;
    std::uint64_t contentHash = 0;
    std::uint32_t compilerVersion = 1;

    [[nodiscard]] CompileId GetId() const noexcept override { return id; }
    [[nodiscard]] CompilerKind GetKind() const noexcept override { return kind; }
    [[nodiscard]] CompileTargetFormat GetFormat() const noexcept override { return format; }
    [[nodiscard]] std::string_view GetKey() const noexcept override { return key; }
    [[nodiscard]] std::span<const std::uint8_t> GetBytes() const noexcept override { return bytes; }
    [[nodiscard]] std::uint64_t GetContentHash() const noexcept override { return contentHash; }
    [[nodiscard]] std::uint32_t GetCompilerVersion() const noexcept override { return compilerVersion; }
};

class ResultImpl final : public ICompileResult {
public:
    CompileStatus status = CompileStatus::Failed;
    CompileId requestId{};
    CompileId jobId{};
    std::shared_ptr<ICompileArtifact> artifact;
    std::vector<CompileDiagnostic> diagnostics;
    std::uint64_t durationMicros = 0;
    bool cacheHit = false;

    [[nodiscard]] CompileStatus GetStatus() const noexcept override { return status; }
    [[nodiscard]] CompileId GetRequestId() const noexcept override { return requestId; }
    [[nodiscard]] CompileId GetJobId() const noexcept override { return jobId; }
    [[nodiscard]] std::shared_ptr<ICompileArtifact> GetArtifact() const override { return artifact; }
    [[nodiscard]] std::span<const CompileDiagnostic> GetDiagnostics() const noexcept override {
        return diagnostics;
    }
    [[nodiscard]] std::uint64_t GetDurationMicros() const noexcept override { return durationMicros; }
    [[nodiscard]] bool WasCacheHit() const noexcept override { return cacheHit; }
};

class RequestImpl final : public ICompileRequest {
public:
    CompileId id{};
    CompilerKind kind = CompilerKind::Unknown;
    CompilePriority priority = CompilePriority::Normal;
    CompileSourceDesc source{};
    CompileOptions options{};
    std::vector<std::string> dependencies;

    [[nodiscard]] CompileId GetId() const noexcept override { return id; }
    [[nodiscard]] CompilerKind GetKind() const noexcept override { return kind; }
    [[nodiscard]] CompilePriority GetPriority() const noexcept override { return priority; }
    [[nodiscard]] const CompileSourceDesc& GetSource() const noexcept override { return source; }
    [[nodiscard]] const CompileOptions& GetOptions() const noexcept override { return options; }
    [[nodiscard]] std::span<const std::string> GetDependencies() const noexcept override {
        return dependencies;
    }
};

class JobImpl final : public ICompileJob {
public:
    CompileId id{};
    CompileId requestId{};
    CompilePriority priority = CompilePriority::Normal;
    std::atomic<CompileStatus> status{CompileStatus::Queued};
    std::atomic<bool> cancel{false};
    std::shared_ptr<RequestImpl> request;

    [[nodiscard]] CompileId GetId() const noexcept override { return id; }
    [[nodiscard]] CompileId GetRequestId() const noexcept override { return requestId; }
    [[nodiscard]] CompileStatus GetStatus() const noexcept override { return status.load(); }
    [[nodiscard]] CompilePriority GetPriority() const noexcept override { return priority; }
    void Cancel() override {
        cancel.store(true);
        status.store(CompileStatus::Cancelled);
    }
};

void RegisterBuiltinCompilers(ICompilerRegistry& registry, const CompilationDependencies& deps);

} // namespace detail
} // namespace we::runtime::compilation
