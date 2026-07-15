#pragma once

#pragma warning(disable: 4251)

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <volk.h>
#include <vector>
#include <string>
#include <memory>

#include "Platform/NativeHandle.h"
#include "Renderer/Export.h"

namespace we::runtime::renderer {

struct DeviceContextConfig {
    we::platform::NativeWindowHandle nativeWindow{};
    std::string appName = "WindEffects";
    bool enableValidationLayers = false;
};

class RENDERER_API DeviceContext {
public:
    DeviceContext() = default;
    ~DeviceContext();

    DeviceContext(const DeviceContext&) = delete;
    DeviceContext& operator=(const DeviceContext&) = delete;

    void Init(const DeviceContextConfig& config);
    void Shutdown();

    VkInstance GetInstance() const;
    VkPhysicalDevice GetPhysicalDevice() const;
    VkDevice GetDevice() const;

    uint32_t GetGraphicsQueueFamily() const;
    VkQueue GetGraphicsQueue() const;

    [[nodiscard]] const we::platform::NativeWindowHandle& GetNativeWindow() const { return m_NativeWindow; }

private:
    void InitVolk();
    void CreateInstance(const DeviceContextConfig& config);
    void PickPhysicalDevice();
    void CreateLogicalDevice();

    we::platform::NativeWindowHandle m_NativeWindow{};
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;

    uint32_t m_GraphicsQueueFamily = ~0u;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;

    bool m_Initialized = false;
};

} // namespace we::runtime::renderer
