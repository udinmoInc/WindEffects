#include "Reflection/Reflection.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    using namespace we::runtime::reflection;

    bool fullScale = false;
    bool writeFiles = true;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--full-scale") {
            fullScale = true;
        } else if (arg == "--no-files") {
            writeFiles = false;
        }
    }

    ProductionReportConfig config;
    config.runTests = true;
    config.runBenchmarks = true;
    config.runFullScaleBenchmarks = fullScale;
    if (!fullScale) {
        config.benchmarkConfig.typeCount = 5000;
        config.benchmarkConfig.objectsPerType = 20;
        config.benchmarkConfig.pluginCount = 25;
        config.benchmarkConfig.inheritanceDepth = 8;
    }

    std::cout << "Running Reflection production hardening suite...\n";
    const ReflectionProductionReport report = GenerateProductionReport(config);
    std::cout << report.textReport << "\n";

    if (writeFiles) {
        const bool wrote = WriteProductionReportFiles(
            report,
            "ReflectionProductionReport.txt",
            "ReflectionProductionReport.json");
        std::cout << (wrote ? "Wrote ReflectionProductionReport.txt/.json\n"
                            : "Failed to write report files\n");
    }

    return report.assessment.productionReady && report.tests.success ? EXIT_SUCCESS : EXIT_FAILURE;
}
