// ==============================================================================
// WindEffects — CrashReporter — IReportProvider
// Internal implementation for the CrashReporter module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
