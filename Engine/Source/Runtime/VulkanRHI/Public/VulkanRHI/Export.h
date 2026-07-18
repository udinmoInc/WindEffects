#pragma once

#if defined(_WIN32)
#if defined(VULKANRHI_EXPORTS)
#define VULKANRHI_API __declspec(dllexport)
#else
#define VULKANRHI_API __declspec(dllimport)
#endif
#else
#define VULKANRHI_API __attribute__((visibility("default")))
#endif
