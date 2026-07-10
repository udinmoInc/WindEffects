#pragma once

#if defined(_WIN32)
#if defined(TEXT_EXPORTS)
#define TEXT_API __declspec(dllexport)
#else
#define TEXT_API __declspec(dllimport)
#endif
#else
#define TEXT_API
#endif
