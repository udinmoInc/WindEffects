#include "Core/FrameCounter.hpp"

namespace we::runtime::core {

std::atomic<uint64_t> FrameCounter::s_FrameNumber{ 0 };

void FrameCounter::Advance() {
    s_FrameNumber.fetch_add(1, std::memory_order_relaxed);
}

uint64_t FrameCounter::GetFrameNumber() {
    return s_FrameNumber.load(std::memory_order_relaxed);
}

void FrameCounter::Reset() {
    s_FrameNumber.store(0, std::memory_order_relaxed);
}

} // namespace we::runtime::core
