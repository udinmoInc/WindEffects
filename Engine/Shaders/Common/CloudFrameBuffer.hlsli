#ifndef WE_CLOUD_FRAME_BUFFER_HLSLI
#define WE_CLOUD_FRAME_BUFFER_HLSLI

// Per-frame cloud UBO (descriptor set 2, b0). Must match CloudFrameUniformData.
cbuffer CloudFrameBuffer : register(b0, space2)
{
    float4x4 cloudPrevViewProj;
    float3   cloudPrevCameraPos;
    float    cloudHistoryValidFlag;
    float2   cloudInvFullResolution;
    float2   cloudInvCloudResolution;
    float2   cloudTemporalJitter;
    float    cloudResolutionScale;
    float    cloudMaxMarchDistance;
    float    cloudTemporalBlendAmt;
    float    cloudEmptySkipThreshold;
    uint     cloudFrameCounter;
    float    cloudDomainRadius; // meters — finite XZ cylinder around worldOrigin
};

#endif // WE_CLOUD_FRAME_BUFFER_HLSLI
