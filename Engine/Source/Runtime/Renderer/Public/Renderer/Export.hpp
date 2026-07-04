#pragma once

#if defined(_WIN32)
#if defined(RENDERER_EXPORTS)
#define RENDERER_API __declspec(dllexport)
#else
#define RENDERER_API __declspec(dllimport)
#endif
#else
#define RENDERER_API
#endif
