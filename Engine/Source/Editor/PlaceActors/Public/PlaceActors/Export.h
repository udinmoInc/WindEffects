#pragma once

#if defined(_WIN32)
#if defined(PLACEACTORS_EXPORTS)
#define PLACEACTORS_API __declspec(dllexport)
#else
#define PLACEACTORS_API __declspec(dllimport)
#endif
#else
#define PLACEACTORS_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
