#pragma once

#if defined(_WIN32)
#if defined(DX12RHI_EXPORTS)
#define DX12RHI_API __declspec(dllexport)
#else
#define DX12RHI_API __declspec(dllimport)
#endif
#else
#define DX12RHI_API __attribute__((visibility("default")))
#endif
