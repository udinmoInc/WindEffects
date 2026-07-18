#pragma once

#if defined(_WIN32)
#if defined(ASSETTOOLS_EXPORTS)
#define ASSETTOOLS_API __declspec(dllexport)
#else
#define ASSETTOOLS_API __declspec(dllimport)
#endif
#else
#define ASSETTOOLS_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
