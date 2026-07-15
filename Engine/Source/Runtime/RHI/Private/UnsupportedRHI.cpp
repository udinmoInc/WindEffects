#include "RHI/UnsupportedRHI.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"

namespace we::rhi {
namespace {

class UnsupportedRHI final : public IRHI {
public:
    UnsupportedRHI(RHIBackend backend, const char* name)
        : m_Backend(backend)
        , m_Name(name ? name : ToString(backend)) {}

    bool Initialize(const RHIInitDesc& desc = {}) override {
        (void)desc;
        m_Initialized = true;
        WE_LOG_WARN(we::LogCategory::Startup,
            std::string("RHI backend '") + m_Name + "' is a stub (NotSupported).");
        return true;
    }

    void Shutdown() override { m_Initialized = false; }
    [[nodiscard]] bool IsInitialized() const override { return m_Initialized; }
    [[nodiscard]] RHIBackend GetActiveBackend() const override { return m_Backend; }
    [[nodiscard]] const char* GetBackendName() const override { return m_Name; }

    [[nodiscard]] std::vector<AdapterDesc> EnumerateAdapters() const override { return {}; }

    [[nodiscard]] RHIResult<std::unique_ptr<IRHIDevice>> CreateDevice(const DeviceDesc&) override {
        return RHIError::Make(
            RHIErrorCode::NotSupported,
            std::string(m_Name) + " backend is not implemented yet.",
            "CreateDevice");
    }

    [[nodiscard]] const RHICapabilities& GetCapabilities() const override { return m_Caps; }

private:
    RHIBackend m_Backend = RHIBackend::Null;
    const char* m_Name = "Unsupported";
    bool m_Initialized = false;
    RHICapabilities m_Caps{};
};

} // namespace

std::unique_ptr<IRHI> CreateUnsupportedRHI(RHIBackend backend, const char* name) {
    return std::make_unique<UnsupportedRHI>(backend, name);
}

} // namespace we::rhi
