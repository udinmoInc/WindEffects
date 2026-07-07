#pragma once

#if defined(_WIN32)
#if defined(EDITORGRIDRENDERER_EXPORTS)
#define EDITORGRIDRENDERER_API __declspec(dllexport)
#else
#define EDITORGRIDRENDERER_API __declspec(dllimport)
#endif
#else
#define EDITORGRIDRENDERER_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
