#include "Compilation/CompilationTests.h"
#include "Compilation/Compilation.h"

#include <filesystem>
#include <sstream>

namespace we::runtime::compilation {
namespace {

void AddCase(CompilationTestReport& report, std::string name, bool passed, std::string message) {
    CompilationTestCaseResult result;
    result.name = std::move(name);
    result.passed = passed;
    result.message = std::move(message);
    report.cases.push_back(std::move(result));
    if (passed) {
        ++report.passed;
    } else {
        ++report.failed;
    }
}

} // namespace

CompilationTestReport RunCompilationRuntimeTests() {
    CompilationTestReport report{};
    CompilationDiagnostics::Get().Reset();

    CompilationDependencies deps;
    deps.config.enableBackgroundCompilation = false; // deterministic sync tick
    deps.config.workerCount = 2;
    deps.config.databasePath =
        (std::filesystem::temp_directory_path() / "we_compilation_test.db").string();

    auto runtime = CreateCompilationRuntime(deps);
    AddCase(report, "CreateCompilationRuntime", runtime != nullptr, runtime ? "ok" : "null");
    if (!runtime) {
        report.success = false;
        report.summary = "Failed to create CompilationRuntime";
        return report;
    }

    runtime->RegisterBuiltinCompilers();
    auto& mgr = runtime->Manager();
    const auto kinds = mgr.Registry().ListKinds();
    AddCase(report, "Builtin compilers registered", kinds.size() >= 10, std::to_string(kinds.size()));

    CompileSourceDesc src;
    src.path = "Shaders/Test.hlsl";
    src.inlineBytes = {'f', 'l', 'o', 'a', 't', '4', ' ', 'm', 'a', 'i', 'n', '(', ')'};
    CompileOptions opts;
    opts.target = CompileTargetFormat::SpirV;
    opts.entryPoint = "main";
    opts.stage = "PS";

    auto req = mgr.CreateRequest(CompilerKind::ShaderHLSL, src, opts, CompilePriority::High);
    auto result = mgr.CompileSync(req);
    AddCase(report, "CompileSync succeed", result && result->GetStatus() == CompileStatus::Succeeded,
        result ? std::to_string(static_cast<int>(result->GetStatus())) : "null");
    AddCase(report, "Artifact produced", result && result->GetArtifact() != nullptr, "artifact");

    auto result2 = mgr.CompileSync(req);
    AddCase(report, "Cache hit", result2 && result2->WasCacheHit(),
        result2 ? (result2->WasCacheHit() ? "hit" : "miss") : "null");

    CompileSourceDesc failSrc;
    failSrc.path = "Shaders/FAIL_Test.hlsl";
    auto failReq = mgr.CreateRequest(CompilerKind::ShaderHLSL, failSrc, opts);
    auto failResult = mgr.CompileSync(failReq);
    AddCase(report, "Compile failure path",
        failResult && failResult->GetStatus() == CompileStatus::Failed, "fail");

    auto matReq = mgr.CreateRequest(CompilerKind::Material, src, {});
    auto matResult = mgr.CompileSync(matReq);
    AddCase(report, "Material compiler",
        matResult && matResult->GetStatus() == CompileStatus::Succeeded, "material");

    bool notified = false;
    mgr.Notifications().Subscribe([&](const ICompileResult&) { notified = true; });
    auto notifyReq = mgr.CreateRequest(CompilerKind::Compute, src, opts);
    (void)mgr.CompileSync(notifyReq);
    AddCase(report, "Notification fired", notified, notified ? "ok" : "missing");

    mgr.Dependencies().AddEdge("a.hlsl", "b.hlsli");
    mgr.Dependencies().AddEdge("b.hlsli", "c.hlsli");
    AddCase(report, "No cycle", !mgr.Dependencies().HasCycle(), "acyclic");
    mgr.Dependencies().AddEdge("c.hlsli", "a.hlsl");
    AddCase(report, "Cycle detected", mgr.Dependencies().HasCycle(), "cyclic");

    const bool saved = mgr.Database().SaveToDisk(deps.config.databasePath);
    AddCase(report, "Database save", saved, saved ? "ok" : "fail");

    std::vector<std::shared_ptr<ICompileRequest>> batch;
    for (int i = 0; i < 8; ++i) {
        CompileSourceDesc bsrc;
        bsrc.path = "batch_" + std::to_string(i) + ".hlsl";
        bsrc.inlineBytes = {static_cast<std::uint8_t>(i)};
        batch.push_back(mgr.CreateRequest(CompilerKind::UiShader, bsrc, opts, CompilePriority::Normal));
    }
    const auto ids = mgr.CompileBatch(std::move(batch), true);
    AddCase(report, "Batch compile", ids.size() == 8, std::to_string(ids.size()));

    // Stress: many unique sync compiles + progress sanity
    std::uint32_t stressOk = 0;
    for (int i = 0; i < 64; ++i) {
        CompileSourceDesc ssrc;
        ssrc.path = "stress_" + std::to_string(i) + ".hlsl";
        ssrc.inlineBytes = {static_cast<std::uint8_t>(i), static_cast<std::uint8_t>(i ^ 0x5A)};
        auto sreq = mgr.CreateRequest(CompilerKind::SpirV, ssrc, opts);
        auto sres = mgr.CompileSync(sreq);
        if (sres && sres->GetStatus() == CompileStatus::Succeeded) {
            ++stressOk;
        }
    }
    AddCase(report, "Stress 64 sync compiles", stressOk == 64, std::to_string(stressOk));
    AddCase(report, "Cache entries > 0", mgr.Cache().EntryCount() > 0, std::to_string(mgr.Cache().EntryCount()));

    runtime->Shutdown();
    std::error_code ec;
    std::filesystem::remove(deps.config.databasePath, ec);

    report.success = report.failed == 0;
    std::ostringstream oss;
    oss << "Compilation tests: " << report.passed << " passed, " << report.failed << " failed";
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::compilation
