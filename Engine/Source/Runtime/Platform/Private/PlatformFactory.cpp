// ==============================================================================
// WindEffects — Platform — PlatformFactory
// Internal implementation for the Platform module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
