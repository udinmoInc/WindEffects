#pragma once

#if defined(_WIN32)
#if defined(SERIALIZATION_EXPORTS)
#define SERIALIZATION_API __declspec(dllexport)
#else
#define SERIALIZATION_API __declspec(dllimport)
#endif
#else
#define SERIALIZATION_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
