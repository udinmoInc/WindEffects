#include "KindUI/Core/DPIContext.h"

#include <atomic>

namespace we::runtime::kindui {

namespace {

// UI-thread primary writer; atomic allows safe concurrent reads (e.g. layout helpers).
std::atomic<float> s_DPIScale{1.0f};

} // namespace

float DPIContext::GetScale() {
    return s_DPIScale.load(std::memory_order_relaxed);
}

void DPIContext::SetScale(float scale) {
    s_DPIScale.store(scale, std::memory_order_relaxed);
}

} // namespace we::runtime::kindui
