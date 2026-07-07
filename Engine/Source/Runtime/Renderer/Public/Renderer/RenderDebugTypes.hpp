#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::renderer {

struct RenderDebuggerLUTStats {
    std::string name;
    uint32_t width = 0;
    uint32_t height = 0;
    std::string format = "R32G32B32A32_SFLOAT";
    float minR = 0.0f, minG = 0.0f, minB = 0.0f;
    float maxR = 0.0f, maxG = 0.0f, maxB = 0.0f;
    float avgR = 0.0f, avgG = 0.0f, avgB = 0.0f;
    uint32_t nanCount = 0;
    uint32_t infCount = 0;
    uint32_t negativeCount = 0;
    bool valid = false;
};

struct RenderDebuggerFirstFailure {
    bool detected = false;
    int pass = -1;
    std::string passName;
    std::string shader;
    std::string sourceFile;
    std::string function;
    std::string variable;
    std::string expectedValue;
    std::string actualValue;
    std::string reason;
    std::string minimalFix;
};

struct GpuCachedLUTData {
    std::string name;
    std::vector<float> rgba;
    uint32_t width = 0;
    uint32_t height = 0;
};

} // namespace we::runtime::renderer
