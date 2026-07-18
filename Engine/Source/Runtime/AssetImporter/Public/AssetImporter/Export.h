#pragma once

#if defined(_WIN32)
#if defined(ASSETIMPORTER_EXPORTS)
#define ASSETIMPORTER_API __declspec(dllexport)
#else
#define ASSETIMPORTER_API __declspec(dllimport)
#endif
#else
#define ASSETIMPORTER_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
