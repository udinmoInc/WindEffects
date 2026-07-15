#pragma once

// Compile-time platform selection. Exactly one of WE_PLATFORM_* is 1.

#if defined(_WIN32)
#define WE_PLATFORM_WINDOWS 1
#define WE_PLATFORM_LINUX   0
#define WE_PLATFORM_MAC     0
#define WE_PLATFORM_ANDROID 0
#define WE_PLATFORM_IOS     0
#define WE_PLATFORM_NAME    "Windows"
#elif defined(__ANDROID__)
#define WE_PLATFORM_WINDOWS 0
#define WE_PLATFORM_LINUX   0
#define WE_PLATFORM_MAC     0
#define WE_PLATFORM_ANDROID 1
#define WE_PLATFORM_IOS     0
#define WE_PLATFORM_NAME    "Android"
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE || TARGET_OS_IOS
#define WE_PLATFORM_WINDOWS 0
#define WE_PLATFORM_LINUX   0
#define WE_PLATFORM_MAC     0
#define WE_PLATFORM_ANDROID 0
#define WE_PLATFORM_IOS     1
#define WE_PLATFORM_NAME    "iOS"
#else
#define WE_PLATFORM_WINDOWS 0
#define WE_PLATFORM_LINUX   0
#define WE_PLATFORM_MAC     1
#define WE_PLATFORM_ANDROID 0
#define WE_PLATFORM_IOS     0
#define WE_PLATFORM_NAME    "Mac"
#endif
#elif defined(__linux__)
#define WE_PLATFORM_WINDOWS 0
#define WE_PLATFORM_LINUX   1
#define WE_PLATFORM_MAC     0
#define WE_PLATFORM_ANDROID 0
#define WE_PLATFORM_IOS     0
#define WE_PLATFORM_NAME    "Linux"
#else
#error "Unsupported platform for WindEffects Platform Layer"
#endif
