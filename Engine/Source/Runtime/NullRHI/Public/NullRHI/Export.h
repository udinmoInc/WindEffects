#pragma once

#if defined(_WIN32)
#if defined(NULLRHI_EXPORTS)
#define NULLRHI_API __declspec(dllexport)
#else
#define NULLRHI_API __declspec(dllimport)
#endif
#else
#define NULLRHI_API __attribute__((visibility("default")))
#endif
