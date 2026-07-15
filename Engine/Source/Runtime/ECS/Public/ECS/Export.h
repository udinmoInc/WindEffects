#pragma once

#if defined(_WIN32)
#if defined(ECS_EXPORTS)
#define ECS_API __declspec(dllexport)
#else
#define ECS_API __declspec(dllimport)
#endif
#else
#define ECS_API
#endif
