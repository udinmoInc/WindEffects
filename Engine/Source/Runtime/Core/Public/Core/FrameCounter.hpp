#pragma once

#include "Core/Export.hpp"
#include <atomic>
#include <cstdint>

namespace we::runtime::core {

class FrameCounter {
public:
    CORE_API static void Advance();
    CORE_API static uint64_t GetFrameNumber();
    CORE_API static void Reset();

private:
    static std::atomic<uint64_t> s_FrameNumber;
};

} // namespace we::runtime::core
