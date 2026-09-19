#ifndef WE_MATH_HLSLI
#define WE_MATH_HLSLI

#include "Platform.hlsli"

static const float WE_PI = 3.14159265359;

float WE_Saturate(float v) { return saturate(v); }

float WE_Determinant4x4(float4x4 m)
{
    float s0 = m[0][0] * m[1][1] - m[1][0] * m[0][1];
    float s1 = m[0][0] * m[1][2] - m[1][0] * m[0][2];
    float s2 = m[0][0] * m[1][3] - m[1][0] * m[0][3];
    float s3 = m[0][1] * m[1][2] - m[1][1] * m[0][2];
    float s4 = m[0][1] * m[1][3] - m[1][1] * m[0][3];
    float s5 = m[0][2] * m[1][3] - m[1][2] * m[0][3];

    float c0 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
    float c1 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
    float c2 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
    float c3 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
    float c4 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
    float c5 = m[2][2] * m[3][3] - m[3][2] * m[2][3];

    return s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
}

float4x4 WE_Inverse4x4(float4x4 m)
{
    float s0 = m[0][0] * m[1][1] - m[1][0] * m[0][1];
    float s1 = m[0][0] * m[1][2] - m[1][0] * m[0][2];
    float s2 = m[0][0] * m[1][3] - m[1][0] * m[0][3];
    float s3 = m[0][1] * m[1][2] - m[1][1] * m[0][2];
    float s4 = m[0][1] * m[1][3] - m[1][1] * m[0][3];
    float s5 = m[0][2] * m[1][3] - m[1][2] * m[0][3];

    float c0 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
    float c1 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
    float c2 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
    float c3 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
    float c4 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
    float c5 = m[2][2] * m[3][3] - m[3][2] * m[2][3];

    float det = s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
    float invDet = 1.0 / det;

    float4x4 inv;
    inv[0][0] = ( m[1][1] * c5 - m[1][2] * c4 + m[1][3] * c3) * invDet;
    inv[0][1] = (-m[0][1] * c5 + m[0][2] * c4 - m[0][3] * c3) * invDet;
    inv[0][2] = ( m[3][1] * s5 - m[3][2] * s4 + m[3][3] * s3) * invDet;
    inv[0][3] = (-m[2][1] * s5 + m[2][2] * s4 - m[2][3] * s3) * invDet;

    inv[1][0] = (-m[1][0] * c5 + m[1][2] * c2 - m[1][3] * c1) * invDet;
    inv[1][1] = ( m[0][0] * c5 - m[0][2] * c2 + m[0][3] * c1) * invDet;
    inv[1][2] = (-m[3][0] * s5 + m[3][2] * s2 - m[3][3] * s1) * invDet;
    inv[1][3] = ( m[2][0] * s5 - m[2][2] * s2 + m[2][3] * s1) * invDet;

    inv[2][0] = ( m[1][0] * c4 - m[1][1] * c2 + m[1][3] * c0) * invDet;
    inv[2][1] = (-m[0][0] * c4 + m[0][1] * c2 - m[0][3] * c0) * invDet;
    inv[2][2] = ( m[3][0] * s4 - m[3][1] * s2 + m[3][3] * s0) * invDet;
    inv[2][3] = (-m[2][0] * s4 + m[2][1] * s2 - m[2][3] * s0) * invDet;

    inv[3][0] = (-m[1][0] * c3 + m[1][1] * c1 - m[1][2] * c0) * invDet;
    inv[3][1] = ( m[0][0] * c3 - m[0][1] * c1 + m[0][2] * c0) * invDet;
    inv[3][2] = (-m[3][0] * s3 + m[3][1] * s1 - m[3][2] * s0) * invDet;
    inv[3][3] = ( m[2][0] * s3 - m[2][1] * s1 + m[2][2] * s0) * invDet;

    return inv;
}

float3 WE_UnprojectPoint(float x, float y, float z, float4x4 view, float4x4 proj)
{
    float4x4 invView = WE_Inverse4x4(view);
    float4x4 invProj = WE_Inverse4x4(proj);
    float4 p = mul(invView, mul(invProj, float4(x, y, z, 1.0)));
    return p.xyz / p.w;
}

// Fullscreen UV → clip/NDC for Vulkan (Y+ down in NDC / framebuffer).
// Must match ProceduralSky: unproject float4(clip.xy, 1, 1) with the same InvVP.
// Pair with VS: o.uv = float2(pos.x * 0.5 + 0.5, pos.y * 0.5 + 0.5) where
//   pos = uvRaw * float2(2, -2) + float2(-1, 1).
// FB top  → uv.y=0 → ndc.y=-1 → world-up rays (verified: yFanOk).
// FB bottom → uv.y=1 → ndc.y=+1 → world-down rays.
// Do NOT use (1 - 2*uv.y): that inverts the vertical ray fan vs InvVP.
float2 WE_UvToNdc(float2 uv)
{
    return float2(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0);
}

// World-space view ray. Uses the far plane (depth 1.0 for perspectiveRH_ZO) so the
// direction fans correctly across the FOV. cameraWorldPos must match the view matrix eye.
float3 WE_UnprojectDirection(float2 uv, float4x4 view, float4x4 proj, float3 cameraWorldPos)
{
    const float2 ndc = WE_UvToNdc(uv);
    const float3 farPoint = WE_UnprojectPoint(ndc.x, ndc.y, 1.0, view, proj);
    return normalize(farPoint - cameraWorldPos);
}

// Same ray via CPU-side inverse(viewProj). Prefer when shader inverse of a large-far
// projection is unreliable. Must stay in sync with ProceduralSky / EditorGrid NDC.
float3 WE_UnprojectDirectionInv(float2 uv, float4x4 invViewProj, float3 cameraWorldPos)
{
    const float2 clipXY = WE_UvToNdc(uv);
    float4 world = mul(invViewProj, float4(clipXY, 1.0, 1.0));
    return normalize(world.xyz / max(world.w, 1e-6) - cameraWorldPos);
}

#endif // WE_MATH_HLSLI
