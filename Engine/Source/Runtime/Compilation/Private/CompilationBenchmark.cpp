#include "Compilation/CompilationBenchmark.h"
#include "Compilation/Compilation.h"

#include <algorithm>
#include <chrono>
#include <sstream>
#include <thread>

namespace we::runtime::compilation {

CompilationBenchmarkReport RunCompilationBenchmarks(
    std::uint32_t syncIterations,
    std::uint32_t parallelJobs)
{
    CompilationBenchmarkReport report{};
    report.syncIterations = syncIterations;
    report.cacheHitIterations = syncIterations;
    report.parallelJobs = parallelJobs;

    CompilationDependencies deps;
    deps.config.enableBackgroundCompilation = true;
    deps.config.workerCount = std::max(2u, std::thread::hardware_concurrency() / 2);
    auto runtime = CreateCompilationRuntime(deps);
    runtime->RegisterBuiltinCompilers();
    auto& mgr = runtime->Manager();

    using clock = std::chrono::steady_clock;

    CompileSourceDesc src;
    src.path = "bench.hlsl";
    src.inlineBytes = {'m', 'a', 'i', 'n'};
    CompileOptions opts;
    opts.forceRebuild = true;

    {
        const auto t0 = clock::now();
        for (std::uint32_t i = 0; i < syncIterations; ++i) {
            src.path = "bench_" + std::to_string(i) + ".hlsl";
            auto req = mgr.CreateRequest(CompilerKind::ShaderHLSL, src, opts);
            (void)mgr.CompileSync(req);
        }
        const auto t1 = clock::now();
        report.syncTotalMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    }

    {
        opts.forceRebuild = false;
        src.path = "bench_cache.hlsl";
        auto warm = mgr.CreateRequest(CompilerKind::ShaderHLSL, src, opts);
        (void)mgr.CompileSync(warm);
        const auto t0 = clock::now();
        for (std::uint32_t i = 0; i < syncIterations; ++i) {
            auto req = mgr.CreateRequest(CompilerKind::ShaderHLSL, src, opts);
            (void)mgr.CompileSync(req);
        }
        const auto t1 = clock::now();
        report.cacheHitTotalMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    }

    {
        std::vector<std::shared_ptr<ICompileRequest>> batch;
        opts.forceRebuild = true;
        for (std::uint32_t i = 0; i < parallelJobs; ++i) {
            CompileSourceDesc bsrc;
            bsrc.path = "par_" + std::to_string(i) + ".hlsl";
            bsrc.inlineBytes = {static_cast<std::uint8_t>(i & 0xFF)};
            batch.push_back(mgr.CreateRequest(CompilerKind::Compute, bsrc, opts));
        }
        const auto t0 = clock::now();
        (void)mgr.CompileBatch(std::move(batch), true);
        const auto t1 = clock::now();
        report.parallelTotalMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    }

    runtime->Shutdown();

    std::ostringstream oss;
    oss << "Compilation bench sync=" << report.syncTotalMicros << "us/" << syncIterations
        << " cache=" << report.cacheHitTotalMicros << "us/" << syncIterations
        << " parallel=" << report.parallelTotalMicros << "us/" << parallelJobs;
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::compilation
