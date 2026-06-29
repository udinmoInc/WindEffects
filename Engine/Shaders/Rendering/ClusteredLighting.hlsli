#ifndef WE_CLUSTERED_LIGHTING_HLSLI
#define WE_CLUSTERED_LIGHTING_HLSLI

#include "Lighting.hlsli"

#if defined(WE_FORWARD_PLUS)
// Clustered / tiled forward+ light list (compute pass populates buffers).
struct WE_ClusterIndex { uint offset; uint count; };
#endif

#endif // WE_CLUSTERED_LIGHTING_HLSLI
