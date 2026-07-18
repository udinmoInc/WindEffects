#pragma once

#if defined(_WIN32)
#if defined(ASSETPROCESSORS_EXPORTS)
#define ASSETPROCESSORS_API __declspec(dllexport)
#else
#define ASSETPROCESSORS_API __declspec(dllimport)
#endif
#else
#define ASSETPROCESSORS_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
