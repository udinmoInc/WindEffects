#pragma once

#pragma warning(disable: 4251)

#include <volk.h>
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <memory>

#include "Renderer/Export.h"

namespace we::runtime::renderer {

struct DeviceContextConfig {
    SDL_Window* window = nullptr;
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
