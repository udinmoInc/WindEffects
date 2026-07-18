#pragma once

#include "AssetImporter/AssetGuid.h"
#include "AssetRuntime/Export.h"
#include "AssetRuntime/RuntimeTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::assetruntime {

struct ASSETRUNTIME_API AssetResidencyInfo {
    we::runtime::assetimporter::AssetGuid guid{};
    std::string virtualPath;
    AssetLoadState state = AssetLoadState::Unloaded;
    uint32_t refCount = 0;
    uint64_t residentBytes = 0;
    uint64_t lastLoadMicroseconds = 0;
    std::string mountId;
};

struct ASSETRUNTIME_API AssetStreamMetrics {
    uint64_t requestsSubmitted = 0;
    uint64_t requestsCompleted = 0;
    uint64_t requestsFailed = 0;
    uint64_t requestsCancelled = 0;
    uint64_t bytesStreamed = 0;
    uint64_t peakQueueDepth = 0;
    uint32_t activeWorkers = 0;
    uint32_t pendingJobs = 0;
};

struct ASSETRUNTIME_API AssetCacheStats {
    uint64_t catalogEntries = 0;
    uint64_t residentAssets = 0;
    uint64_t residentBytes = 0;
    uint64_t memoryBudgetBytes = 0;
    uint64_t evictions = 0;
    uint64_t cacheHits = 0;
    uint64_t cacheMisses = 0;
    uint64_t syncLoads = 0;
    uint64_t asyncLoads = 0;
    uint64_t hotReloads = 0;
    uint64_t failedLoads = 0;
    double averageLoadMilliseconds = 0.0;
    double peakLoadMilliseconds = 0.0;
};

struct ASSETRUNTIME_API PackageMountInfo {
    std::string mountId;
    std::string providerId;
    std::string virtualRoot;
    std::string sourcePath;
    uint32_t packageVersion = 0;
    int priority = 0;
    uint64_t assetCount = 0;
    bool readOnly = true;
};

struct ASSETRUNTIME_API AssetManagerDiagnostics {
    AssetCacheStats cache{};
    AssetStreamMetrics streaming{};
    std::vector<PackageMountInfo> mounts;
    std::vector<AssetResidencyInfo> residency; // optional snapshot; may be empty unless requested
};

} // namespace we::runtime::assetruntime
