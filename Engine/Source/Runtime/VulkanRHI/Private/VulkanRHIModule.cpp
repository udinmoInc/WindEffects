#include "VulkanDevice.h"

#include "Modules/IModuleInterface.h"
#include "RHI/RHIFactory.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"

#include <memory>
#include <vector>

namespace we::rhi {
namespace {

class VulkanRHIBackend final : public IRHI {
public:
    bool Initialize(const RHIInitDesc& desc = {}) override {
        m_Desc = desc;
        m_Caps.dynamicRendering = true;
        m_Caps.debugMarkers = true;
        m_Caps.maxFramesInFlight = desc.framesInFlight ? desc.framesInFlight : 2;
        m_Initialized = true;
        WE_LOG_INFO(we::LogCategory::Startup, "VulkanRHI initialized.");
        return true;
    }

    void Shutdown() override {
        m_Initialized = false;
        WE_LOG_INFO(we::LogCategory::Startup, "VulkanRHI shut down.");
    }

    [[nodiscard]] bool IsInitialized() const override { return m_Initialized; }
    [[nodiscard]] RHIBackend GetActiveBackend() const override { return RHIBackend::Vulkan; }
    [[nodiscard]] const char* GetBackendName() const override { return "Vulkan"; }

    [[nodiscard]] std::vector<AdapterDesc> EnumerateAdapters() const override {
        // Lightweight placeholder until device creation enumerates; filled after volk init in CreateDevice path.
        AdapterDesc a{};
        a.index = 0;
        a.name = "Vulkan Adapter";
        a.isDiscrete = true;
        return {a};
    }

    [[nodiscard]] RHIResult<std::unique_ptr<IRHIDevice>> CreateDevice(const DeviceDesc& desc) override {
        if (!m_Initialized) {
            return RHIError::Make(RHIErrorCode::NotInitialized, "VulkanRHI not initialized.", "CreateDevice");
        }
        DeviceDesc resolved = desc;
        if (m_Desc.enableValidation) {
            resolved.enableValidation = true;
        }
        if (resolved.framesInFlight == 0) {
            resolved.framesInFlight = m_Desc.framesInFlight ? m_Desc.framesInFlight : 2;
        }
        auto device = vulkan::CreateVulkanDevice(resolved);
        if (!device) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to create Vulkan device.", "CreateDevice");
        }
        return std::unique_ptr<IRHIDevice>(std::move(device));
    }

    [[nodiscard]] const RHICapabilities& GetCapabilities() const override { return m_Caps; }

private:
    bool m_Initialized = false;
    RHIInitDesc m_Desc{};
    RHICapabilities m_Caps{};
};

std::unique_ptr<IRHI> CreateVulkanRHI() {
    return std::make_unique<VulkanRHIBackend>();
}

static RHIBackendRegistrar g_VulkanRHIRegistrar(RHIBackend::Vulkan, &CreateVulkanRHI, "Vulkan");

} // namespace

class VulkanRHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        RHIFactory::Register(RHIBackend::Vulkan, &CreateVulkanRHI, "Vulkan");
    }

    void ShutdownModule() override {}
};

} // namespace we::rhi

IMPLEMENT_MODULE(we::rhi::VulkanRHIModule, WindEffects_VulkanRHI)
