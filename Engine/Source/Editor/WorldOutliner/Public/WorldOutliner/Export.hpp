#pragma once

#if defined(_WIN32)
#if defined(WORLDOUTLINER_EXPORTS)
#define WORLDOUTLINER_API __declspec(dllexport)
#else
#define WORLDOUTLINER_API __declspec(dllimport)
#endif
#else
#define WORLDOUTLINER_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
