#pragma once

#if defined(_WIN32)
#if defined(ASSETRUNTIME_EXPORTS)
#define ASSETRUNTIME_API __declspec(dllexport)
#else
#define ASSETRUNTIME_API __declspec(dllimport)
#endif
#else
#define ASSETRUNTIME_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
