#pragma once
#include <string>
#include <functional>

namespace we::programs::crashreporter {

struct ReportData {
    std::string crashDir;
    std::string zipFilePath;
    std::string userComments;
    bool includeLogs;
    bool includeDump;
    bool includeScreenshot;
    bool includeSystemInfo;
};

class IReportProvider {
public:
    virtual ~IReportProvider() = default;

    // Callback parameter is for progress (0.0 to 1.0) and status message.
    // Returns true if successfully submitted.
    virtual bool SubmitReport(const ReportData& payload, std::function<void(float, const std::string&)> progressCallback) = 0;
};

} // namespace we::programs::crashreporter
