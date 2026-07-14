#ifndef WE_COMMON_CAMERA_BUFFER_HLSLI
#define WE_COMMON_CAMERA_BUFFER_HLSLI

// Shared canonical CameraBuffer layout.
// Must match we::runtime::renderer::CameraUniform (CPU):
//   view (float4x4)
//   proj (float4x4)
//   invViewProj (float4x4)
//   cameraPos (float3) + cameraPadding (float)
//   Foundation sky reuses cameraPadding as skyDebugMode (see ProceduralSky.hlsl).
//
// Shaders may override the descriptor-set ("space") via WE_CAMERA_BUFFER_SPACE.
#ifndef WE_CAMERA_BUFFER_SPACE
#define WE_CAMERA_BUFFER_SPACE space0
#endif

cbuffer CameraBuffer : register(b0, WE_CAMERA_BUFFER_SPACE)
{
    float4x4 view;
    float4x4 proj;
    float4x4 invViewProj;
    float3 cameraPos;
    float cameraPadding;
};

#endif // WE_COMMON_CAMERA_BUFFER_HLSLI

