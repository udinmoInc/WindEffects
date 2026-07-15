#include "Platform/PlatformFactory.h"
#include "Platform/PlatformConfig.h"

#if WE_PLATFORM_WINDOWS
#include "Windows/WindowsPlatform.h"
#elif WE_PLATFORM_LINUX
#include "Linux/LinuxPlatform.h"
#elif WE_PLATFORM_MAC
#include "Mac/MacPlatform.h"
#elif WE_PLATFORM_ANDROID
#include "Android/AndroidPlatform.h"
#elif WE_PLATFORM_IOS
#include "IOS/IOSPlatform.h"
#endif

namespace we::platform {

IPlatform* PlatformFactory::Create() {
#if WE_PLATFORM_WINDOWS
    return new WindowsPlatform();
#elif WE_PLATFORM_LINUX
    return new LinuxPlatform();
#elif WE_PLATFORM_MAC
    return new MacPlatform();
#elif WE_PLATFORM_ANDROID
    return new AndroidPlatform();
#elif WE_PLATFORM_IOS
    return new IOSPlatform();
#else
    return nullptr;
#endif
}

const char* PlatformFactory::GetHostPlatformName() noexcept {
    return WE_PLATFORM_NAME;
}

} // namespace we::platform
