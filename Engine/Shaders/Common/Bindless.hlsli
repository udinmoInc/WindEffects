#ifndef WE_BINDLESS_HLSLI
#define WE_BINDLESS_HLSLI

#include "Platform.hlsli"

#if defined(WE_BINDLESS)
// Bindless descriptor indexing (SM 6.6+), enabled when WE_BINDLESS permutation is set.
#define WE_BINDLESS_SPACE space2
#else
#define WE_BINDLESS_ENABLED 0
#endif

#endif // WE_BINDLESS_HLSLI
