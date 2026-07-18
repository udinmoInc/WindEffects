#include "Serialization/SerializationTests.h"
#include "Serialization/Types.h"

#include <fstream>
#include <sstream>

namespace we::runtime::serialization {
namespace {

std::string EscapeJson(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (char c : text) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

std::string BuildText(const SerializationProductionReport& report) {
    std::ostringstream oss;
    oss << "================================================================\n";
    oss << " WindEffects Serialization Runtime — Production Report\n";
    oss << "================================================================\n\n";
    oss << "ABI version: " << kSerializationAbiVersion << "\n";
    oss << "Format version: " << kSerializationFormatVersion << "\n\n";

    oss << "--- Automated Tests ---\n";
    oss << report.tests.summary << "\n";
    for (const auto& testCase : report.tests.cases) {
        if (!testCase.passed) {
            oss << "  FAIL " << testCase.name << " — " << testCase.message << "\n";
        }
    }
    oss << "\n--- Benchmarks ---\n";
    oss << report.benchmark.summary << "\n";
    for (const auto& sample : report.benchmark.samples) {
        oss << "  " << sample.name << ": " << sample.milliseconds << " ms ("
            << sample.iterations << " iters, " << sample.perIterationNs << " ns/op)\n";
    }
    oss << "\n--- Diagnostics ---\n";
    oss << "Document bytes (last graph): " << report.benchmark.diagnostics.memory.documentBytes << "\n";
    oss << "Checksum valid: " << (report.benchmark.diagnostics.checksumValid ? "yes" : "no") << "\n";
    oss << "\n--- Production Readiness ---\n";
    oss << report.verdict << "\n";
    return oss.str();
}

std::string BuildJson(const SerializationProductionReport& report) {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"abiVersion\": " << kSerializationAbiVersion << ",\n";
    oss << "  \"formatVersion\": " << kSerializationFormatVersion << ",\n";
    oss << "  \"tests\": {\n";
    oss << "    \"passed\": " << report.tests.passed << ",\n";
    oss << "    \"failed\": " << report.tests.failed << ",\n";
    oss << "    \"success\": " << (report.tests.success ? "true" : "false") << "\n";
    oss << "  },\n";
    oss << "  \"productionReady\": " << (report.productionReady ? "true" : "false") << ",\n";
    oss << "  \"verdict\": \"" << EscapeJson(report.verdict) << "\"\n";
    oss << "}\n";
    return oss.str();
}

} // namespace

SerializationProductionReport GenerateSerializationProductionReport(
    const SerializationProductionReportConfig& config)
{
    SerializationProductionReport report;
    if (config.runTests) {
        report.tests = RunSerializationTests();
    }
    if (config.runBenchmarks) {
        SerializationBenchmarkConfig bench = config.benchmarkConfig;
        if (config.runFullScaleBenchmarks) {
            bench.runFullScale = true;
        }
        report.benchmark = RunSerializationBenchmarks(bench);
        report.diagnostics = report.benchmark.diagnostics;
    }
    report.productionReady = report.tests.success && report.benchmark.success;
    report.verdict = report.productionReady
        ? "PRODUCTION READY — Serialization Runtime is the permanent binary persistence foundation for World, Scene, Prefabs, Assets, Undo/Redo, Networking, and Save Games."
        : "NOT READY — see failed tests / benchmarks.";
    report.textReport = BuildText(report);
    report.jsonReport = BuildJson(report);
    return report;
}

bool WriteSerializationProductionReportFiles(
    const SerializationProductionReport& report,
    std::string_view textPath,
    std::string_view jsonPath)
{
    bool ok = true;
    if (!textPath.empty()) {
        std::ofstream text(std::string(textPath), std::ios::binary);
        if (!text) {
            ok = false;
        } else {
            text << report.textReport;
        }
    }
    if (!jsonPath.empty()) {
        std::ofstream json(std::string(jsonPath), std::ios::binary);
        if (!json) {
            ok = false;
        } else {
            json << report.jsonReport;
        }
    }
    return ok;
}

} // namespace we::runtime::serialization
