#pragma once

#if defined(_WIN32)
#if defined(CORE_EXPORTS)
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport)
#endif
#else
#define CORE_API
#endif
