#pragma once

// DLL export policy for Renderer module:
// - RENDERER_API marks symbols exported from WERenderer.dll.
// - Declarations belong in headers; definitions belong in exactly one .cpp per symbol.
// - Never place RENDERER_API function bodies in headers (causes duplicate exported symbols).

#if defined(_WIN32)
#if defined(RENDERER_EXPORTS)
#define RENDERER_API __declspec(dllexport)
#else
#define RENDERER_API __declspec(dllimport)
#endif
#else
#define RENDERER_API
#endif
