#include "AssetProcessors/ProcessorRegistry.h"

#include <algorithm>
#include <cstdint>
#include <sstream>

namespace we::runtime::assetprocessors {

void ProcessorRegistry::Register(std::shared_ptr<IAssetProcessor> processor) {
    if (!processor) {
        return;
    }
    std::lock_guard lock(m_Mutex);
    m_ById[std::string(processor->GetProcessorId())] = std::move(processor);
}

void ProcessorRegistry::Unregister(std::string_view processorId) {
    std::lock_guard lock(m_Mutex);
    m_ById.erase(std::string(processorId));
}

void ProcessorRegistry::Clear() {
    std::lock_guard lock(m_Mutex);
    m_ById.clear();
}

std::shared_ptr<IAssetProcessor> ProcessorRegistry::FindById(std::string_view processorId) const {
    std::lock_guard lock(m_Mutex);
    const auto it = m_ById.find(std::string(processorId));
    return it == m_ById.end() ? nullptr : it->second;
}

std::vector<std::shared_ptr<IAssetProcessor>> ProcessorRegistry::GetForStage(ProcessStage stage) const {
    std::vector<std::shared_ptr<IAssetProcessor>> out;
    {
        std::lock_guard lock(m_Mutex);
        for (const auto& [_, p] : m_ById) {
            if (p && p->GetStage() == stage) {
                out.push_back(p);
            }
        }
    }
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        return a->GetPriority() > b->GetPriority();
    });
    return out;
}

std::vector<std::shared_ptr<IAssetProcessor>> ProcessorRegistry::GetAll() const {
    std::lock_guard lock(m_Mutex);
    std::vector<std::shared_ptr<IAssetProcessor>> out;
    out.reserve(m_ById.size());
    for (const auto& [_, p] : m_ById) {
        out.push_back(p);
    }
    return out;
}

std::string ProcessorRegistry::ComputeFingerprint() const {
    auto all = GetAll();
    std::sort(all.begin(), all.end(), [](const auto& a, const auto& b) {
        if (a->GetStage() != b->GetStage()) {
            return static_cast<uint32_t>(a->GetStage()) < static_cast<uint32_t>(b->GetStage());
        }
        return a->GetProcessorId() < b->GetProcessorId();
    });
    std::ostringstream oss;
    for (const auto& p : all) {
        oss << p->GetProcessorId() << '@' << p->GetVersion() << ';'
            << ProcessStageToString(p->GetStage()) << '|';
    }
    // Stable FNV over descriptor string via file-hash helper on a temp is overkill;
    // reuse ComputeFileHashHex pattern inline.
    const std::string s = oss.str();
    uint64_t hash = 1469598103934665603ull;
    for (unsigned char c : s) {
        hash ^= c;
        hash *= 1099511628211ull;
    }
    std::ostringstream hex;
    hex << std::hex << hash;
    return hex.str();
}

} // namespace we::runtime::assetprocessors
