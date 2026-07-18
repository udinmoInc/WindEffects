#include "Serialization/Serialization.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    using namespace we::runtime::serialization;

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

    SerializationProductionReportConfig config;
    config.runTests = true;
    config.runBenchmarks = true;
    config.runFullScaleBenchmarks = fullScale;
    if (!fullScale) {
        config.benchmarkConfig.objectCount = 20000;
        config.benchmarkConfig.graphSize = 2000;
    }

    std::cout << "Running Serialization production hardening suite...\n";
    const SerializationProductionReport report = GenerateSerializationProductionReport(config);
    std::cout << report.textReport << "\n";

    if (writeFiles) {
        const bool wrote = WriteSerializationProductionReportFiles(
            report,
            "SerializationProductionReport.txt",
            "SerializationProductionReport.json");
        std::cout << (wrote ? "Wrote SerializationProductionReport.txt/.json\n"
                            : "Failed to write report files\n");
    }

    return report.productionReady ? EXIT_SUCCESS : EXIT_FAILURE;
}
