#ifndef WE_GPUDRIVEN_HLSLI
#define WE_GPUDRIVEN_HLSLI

#include "../Common/Platform.hlsli"

struct WE_DrawIndexedIndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int  vertexOffset;
    uint firstInstance;
};

#endif // WE_GPUDRIVEN_HLSLI
