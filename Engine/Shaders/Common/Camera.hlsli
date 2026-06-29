#ifndef WE_CAMERA_HLSLI
#define WE_CAMERA_HLSLI

#include "Platform.hlsli"

struct CameraBufferData
{
    float4x4 view;
    float4x4 proj;
    float3   pos;
    float    padding;
};

#endif // WE_CAMERA_HLSLI
