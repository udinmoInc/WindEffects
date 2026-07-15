#include "DX12Device.h"

#include "Modules/IModuleInterface.h"
#include "RHI/RHIFactory.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"

#include <memory>
#include <vector>

#if defined(_WIN32)

namespace we::rhi {
namespace {

class DX12RHIBackend final : public IRHI {
public:
    bool Initialize(const RHIInitDesc& desc = {}) override {
        m_Desc = desc;
        m_Caps.dynamicRendering = true;
        m_Caps.debugMarkers = true;
        m_Caps.timelineSemaphores = true;
        m_Caps.asyncCompute = true;
        m_Caps.textureCompressionBC = true;
        m_Caps.multiDrawIndirect = true;
        m_Caps.geometryShaders = true;
        m_Caps.tessellation = true;
        m_Caps.samplerAnisotropy = true;
        m_Caps.fillModeNonSolid = true;
        m_Caps.maxFramesInFlight = desc.framesInFlight ? desc.framesInFlight : 2;
        m_Caps.maxTextureDimension2D = 16384;
        m_Caps.maxColorAttachments = 8;
        m_Caps.maxPushConstantBytes = 256;
        m_Initialized = true;
        WE_LOG_INFO(we::LogCategory::Startup, "DirectX12RHI initialized.");
        return true;
    }

    void Shutdown() override {
        m_Initialized = false;
        WE_LOG_INFO(we::LogCategory::Startup, "DirectX12RHI shut down.");
    }

    [[nodiscard]] bool IsInitialized() const override { return m_Initialized; }
    [[nodiscard]] RHIBackend GetActiveBackend() const override { return RHIBackend::DirectX12; }
    [[nodiscard]] const char* GetBackendName() const override { return "DirectX12"; }

    [[nodiscard]] std::vector<AdapterDesc> EnumerateAdapters() const override {
        AdapterDesc a{};
        a.index = 0;
        a.name = "DirectX12 Adapter";
        a.isDiscrete = true;
        return {a};
    }

    [[nodiscard]] RHIResult<std::unique_ptr<IRHIDevice>> CreateDevice(const DeviceDesc& desc) override {
        if (!m_Initialized) {
            return RHIError::Make(RHIErrorCode::NotInitialized, "DirectX12RHI not initialized.", "CreateDevice");
        }
        DeviceDesc resolved = desc;
        if (m_Desc.enableValidation) {
            resolved.enableValidation = true;
        }
        if (resolved.framesInFlight == 0) {
            resolved.framesInFlight = m_Desc.framesInFlight ? m_Desc.framesInFlight : 2;
        }
        auto device = dx12::CreateDX12Device(resolved);
        if (!device) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to create DirectX12 device.", "CreateDevice");
        }
        return std::unique_ptr<IRHIDevice>(std::move(device));
    }

    [[nodiscard]] const RHICapabilities& GetCapabilities() const override { return m_Caps; }

private:
    bool m_Initialized = false;
    RHIInitDesc m_Desc{};
    RHICapabilities m_Caps{};
};

std::unique_ptr<IRHI> CreateDirectX12RHI() {
    return std::make_unique<DX12RHIBackend>();
}

static RHIBackendRegistrar g_DirectX12Registrar(RHIBackend::DirectX12, &CreateDirectX12RHI, "DirectX12");

} // namespace

class DirectX12RHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        RHIFactory::Register(RHIBackend::DirectX12, &CreateDirectX12RHI, "DirectX12");
    }
    void ShutdownModule() override {}
};
} // namespace we::rhi

IMPLEMENT_MODULE(we::rhi::DirectX12RHIModule, WindEffects_DirectX12RHI)

#else

#include "Modules/IModuleInterface.h"
#include "RHI/RHIFactory.h"
#include "RHI/UnsupportedRHI.h"

#include <memory>

namespace we::rhi {
namespace {
std::unique_ptr<IRHI> CreateDirectX12RHI() {
    return CreateUnsupportedRHI(RHIBackend::DirectX12, "DirectX12");
}
static RHIBackendRegistrar g_DirectX12Registrar(RHIBackend::DirectX12, &CreateDirectX12RHI, "DirectX12");
} // namespace

class DirectX12RHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        RHIFactory::Register(RHIBackend::DirectX12, &CreateDirectX12RHI, "DirectX12");
    }
    void ShutdownModule() override {}
};
} // namespace we::rhi

IMPLEMENT_MODULE(we::rhi::DirectX12RHIModule, WindEffects_DirectX12RHI)

#endif
