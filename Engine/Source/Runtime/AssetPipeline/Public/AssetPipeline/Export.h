#pragma once

#if defined(_WIN32)
#if defined(ASSETPIPELINE_EXPORTS)
#define ASSETPIPELINE_API __declspec(dllexport)
#else
#define ASSETPIPELINE_API __declspec(dllimport)
#endif
#else
#define ASSETPIPELINE_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
