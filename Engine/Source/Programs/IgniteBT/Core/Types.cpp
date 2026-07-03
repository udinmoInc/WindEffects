#include "Core/Types.hpp"
#include <sstream>
#include <iostream>

namespace IgniteBT {

std::string Version::ToString() const {
    std::ostringstream oss;
    oss << major << "." << minor << "." << patch;
    return oss.str();
}

Version Version::Parse(const std::string& versionString) {
    Version version;
    std::istringstream iss(versionString);
    char dot;
    
    iss >> version.major >> dot >> version.minor >> dot >> version.patch;
    
    return version;
}

bool Version::operator==(const Version& other) const {
    return major == other.major && minor == other.minor && patch == other.patch;
}

bool Version::operator<(const Version& other) const {
    if (major != other.major) return major < other.major;
    if (minor != other.minor) return minor < other.minor;
    return patch < other.patch;
}

double BuildStatistics::GetCacheHitRate() const {
    int total = cacheHits + cacheMisses;
    if (total == 0) return 0.0;
    return static_cast<double>(cacheHits) / static_cast<double>(total);
}

void BuildStatistics::Print() const {
    std::cout << "=== Build Statistics ===" << std::endl;
    std::cout << "Total Targets: " << totalTargets << std::endl;
    std::cout << "Successful: " << successfulTargets << std::endl;
    std::cout << "Failed: " << failedTargets << std::endl;
    std::cout << "Skipped: " << skippedTargets << std::endl;
    std::cout << "Cached: " << cachedTargets << std::endl;
    std::cout << "Total Build Time: " << totalBuildTime.count() << "ms" << std::endl;
    std::cout << "Total Compile Time: " << totalCompileTime.count() << "ms" << std::endl;
    std::cout << "Total Link Time: " << totalLinkTime.count() << "ms" << std::endl;
    std::cout << "Cache Hit Rate: " << (GetCacheHitRate() * 100.0) << "%" << std::endl;
}

} // namespace IgniteBT
