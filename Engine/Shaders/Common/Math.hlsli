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

// Column-vector conventions to match GLM / SPIR-V layouts used by the engine.
// uv must be in [0, 1] (normalize fullscreen-triangle UV by 0.5 when needed).
// cameraWorldPos must match the position used to build the view matrix.
float3 WE_UnprojectDirection(float2 uv, float4x4 view, float4x4 proj, float3 cameraWorldPos)
{
    const float2 ndc = uv * 2.0 - 1.0;
    // Reverse-Z (perspectiveRH_ZO with far/near swapped): far plane at depth 0.
    const float3 farPoint = WE_UnprojectPoint(ndc.x, ndc.y, 0.0, view, proj);
    return normalize(farPoint - cameraWorldPos);
}

#endif // WE_MATH_HLSLI
