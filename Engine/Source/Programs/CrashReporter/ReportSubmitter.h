#pragma once
#include "IReportProvider.h"
#include <memory>
#include <cstdlib>

namespace we::programs::crashreporter {

class ReportSubmitter {
public:
    ReportSubmitter() = default;

    void SetProvider(std::shared_ptr<IReportProvider> provider) {
        m_Provider = provider;
    }

    bool Submit(const ReportData& data, std::function<void(float, const std::string&)> progressCallback) {
        // First compress the payload
        progressCallback(0.1f, "Compressing logs and dump files...");
        
        std::string zipCmd = "powershell.exe -c \"Compress-Archive -Path '" + data.crashDir + "/*' -DestinationPath '" + data.zipFilePath + "' -Force\"";
        if (system(zipCmd.c_str()) != 0) {
            progressCallback(0.0f, "Failed to compress report data.");
            return false;
        }

        progressCallback(0.3f, "Compression complete.");

        if (m_Provider) {
            return m_Provider->SubmitReport(data, [progressCallback](float p, const std::string& msg) {
                // scale provider progress from 0.3 to 1.0
                float scaledP = 0.3f + (p * 0.7f);
                progressCallback(scaledP, msg);
            });
        }
        
        progressCallback(1.0f, "Report generated successfully (No provider configured to send).");
        return true;
    }

private:
    std::shared_ptr<IReportProvider> m_Provider;
};

} // namespace we::programs::crashreporter
