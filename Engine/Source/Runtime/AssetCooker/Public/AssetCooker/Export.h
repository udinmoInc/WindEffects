#pragma once

#if defined(_WIN32)
#if defined(ASSETCOOKER_EXPORTS)
#define ASSETCOOKER_API __declspec(dllexport)
#else
#define ASSETCOOKER_API __declspec(dllimport)
#endif
#else
#define ASSETCOOKER_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
