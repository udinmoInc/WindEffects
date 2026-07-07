#pragma once

#include <volk.h>
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <memory>

namespace we::runtime::renderer {

struct DeviceContextConfig {
    SDL_Window* window = nullptr;
    std::string appName = "WindEffects";
    bool enableValidationLayers = false;
};

class DeviceContext {
public:
    DeviceContext() = default;
    ~DeviceContext();

    DeviceContext(const DeviceContext&) = delete;
    DeviceContext& operator=(const DeviceContext&) = delete;

    void Init(const DeviceContextConfig& config);
    void Shutdown();

    VkInstance GetInstance() const { return m_Instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    VkDevice GetDevice() const { return m_Device; }

    uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
    VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }

private:
    void InitVolk();
    void CreateInstance(const DeviceContextConfig& config);
    void PickPhysicalDevice();
    void CreateLogicalDevice();

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;

    uint32_t m_GraphicsQueueFamily = ~0u;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;

    bool m_Initialized = false;
};

} // namespace we::runtime::renderer
