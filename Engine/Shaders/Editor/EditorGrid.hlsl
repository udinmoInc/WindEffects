#define WE_CAMERA_BUFFER_SPACE space1
#include "../Common/CameraBuffer.hlsli"
#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"

// Blender-style flat XZ editor grid. Fullscreen raycast — full viewport coverage.

cbuffer GridSettings : register(b0, space0)
{
    float4 levelSizes;      // 1, 10, 100, 1000 m
    float4 levelFadeStart;
    float4 levelFadeEnd;

    float4 level0Color;     // minor 1 m   (rgba)
    float4 level1Color;     // medium 10 m
    float4 level2Color;     // large 100 m
    float4 level3Color;     // major 1000 m
    float4 axisXColor;      // rgba
    float4 axisZColor;      // rgba

    float4 renderParams0;   // x=renderRadius, y=planeHeight, z=lineThicknessMinor, w=lineThicknessMajor
    float4 renderParams1;   // x=baseOpacity, y=lineThicknessAxis, z=horizonGuardNdcMargin, w=antiAliasScale
    float4 renderParams2;   // x=depthBiasConstant, y=depthBiasSlope, z=cameraDistance, w=radiusFadeStart
    float4 snappedOrigin;   // xyz = snapped center, w = cull radius

    int4   gridFlags;       // x=enableGrid, y=enableAxis, z=antiAliasingEnabled
    float4 depthParams;     // x=depthOffset, y=radiusFadeEnd, z=distanceFadeStart, w=distanceFadeEnd
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    VSOutput o;
    const float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    const float2 pos = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);
    o.position = float4(pos, 0.0, 1.0);
    o.uv = uv * 0.5;
    return o;
}

float LevelVisibility(float cameraDistance, float fadeStart, float fadeEnd)
{
    if (fadeEnd <= fadeStart + 1e-4)
        return 1.0;
    return 1.0 - smoothstep(fadeStart, fadeEnd, cameraDistance);
}

float LineStrength(float2 worldXZ, float cellSize, float lineWidth, bool useAA)
{
    const float2 uv = worldXZ / max(cellSize, 1e-4);
    const float2 grid = abs(frac(uv - 0.5) - 0.5);
    const float aa = max(renderParams1.w, 0.5);
    const float2 derivative = max(fwidth(uv) * lineWidth * aa, float2(1e-4, 1e-4));

    if (useAA)
    {
        const float lineX = 1.0 - smoothstep(0.0, derivative.x, grid.x);
        const float lineY = 1.0 - smoothstep(0.0, derivative.y, grid.y);
        return saturate(max(lineX, lineY));
    }

    return (grid.x < derivative.x || grid.y < derivative.y) ? 1.0 : 0.0;
}

float AxisLineMask(float axisCoord, float distToAxis, float lineWidthPx, bool useAA)
{
    const float aa = max(renderParams1.w, 0.5);
    // Screen-space constant thickness: threshold in world units is fwidth(axisCoord) * pixels.
    // Clamp pixels so we never widen the axis at distance.
    const float px = clamp(lineWidthPx, 0.75, 1.1);
    const float derivative = max(fwidth(axisCoord) * px * aa, 1e-4);
    if (useAA)
        return saturate(1.0 - smoothstep(0.0, derivative, distToAxis));
    return distToAxis < derivative ? 1.0 : 0.0;
}

struct PSOutput
{
    float4 color : SV_Target0;
    float  depth : SV_Depth;
};

PSOutput PSMain(VSOutput input)
{
    PSOutput o;

    if (gridFlags.x == 0)
        discard;

    const float planeHeight  = renderParams0.y;
    const float depthOffset  = depthParams.x;
    const float groundY      = planeHeight + depthOffset;
    const float renderRadius = renderParams0.x;

    // Per-pixel unprojection — interpolating world-space ray endpoints across the
    // fullscreen triangle bends rays with camera pitch and paints the grid on the sky.
    const float3 viewRayDir = WE_UnprojectDirection(input.uv, view, proj, cameraPos);

    // Sky / above-horizon rays never hit the ground in front of the camera.
    if (viewRayDir.y >= -1e-5)
        discard;

    const float t = (groundY - cameraPos.y) / viewRayDir.y;
    if (t < 0.0)
        discard;

    const float3 fragPos = cameraPos + viewRayDir * t;
    const float2 worldXZ = fragPos.xz;
    const float2 localXZ = worldXZ - float2(snappedOrigin.x, snappedOrigin.z);
    const float radialDist = length(localXZ);
    const float distToCamera = length(fragPos - cameraPos);

    if (radialDist > snappedOrigin.w)
        discard;

    // Screen-space horizon guard: never draw where the ground plane projects above the eye-level horizon.
    const float heightAboveGround = cameraPos.y - groundY;
    if (heightAboveGround > 1e-3)
    {
        const float tHorizon = heightAboveGround / max(-viewRayDir.y, 1e-5);
        const float3 horizonPos = cameraPos + viewRayDir * tHorizon;
        const float4 horizonClip = mul(proj, mul(view, float4(horizonPos, 1.0)));
        const float4 fragClip = mul(proj, mul(view, float4(fragPos, 1.0)));
        if (horizonClip.w > 1e-4 && fragClip.w > 1e-4)
        {
            const float horizonNdcY = horizonClip.y / horizonClip.w;
            const float fragNdcY = fragClip.y / fragClip.w;
            const float horizonMargin = max(renderParams1.z, 0.0);
            if (fragNdcY <= horizonNdcY + horizonMargin)
                discard;
        }
    }

    const bool useAA = (gridFlags.z != 0);
    const float camDist = renderParams2.z;
    const float lineMinor = renderParams0.z;
    const float lineMajor = renderParams0.w;

    const float radiusFadeStart = renderParams2.w * renderRadius;
    const float radiusFadeEnd   = max(depthParams.y * renderRadius, radiusFadeStart + 1.0);
    const float edgeFade = 1.0 - smoothstep(radiusFadeStart, radiusFadeEnd, radialDist);

    const float fade0 = LevelVisibility(camDist, levelFadeStart.x, levelFadeEnd.x);
    const float fade1 = LevelVisibility(camDist, levelFadeStart.y, levelFadeEnd.y);
    const float fade2 = LevelVisibility(camDist, levelFadeStart.z, levelFadeEnd.z);
    const float fade3 = LevelVisibility(camDist, levelFadeStart.w, levelFadeEnd.w);

    const float s0 = LineStrength(worldXZ, levelSizes.x, lineMinor, useAA) * fade0;
    const float s1 = LineStrength(worldXZ, levelSizes.y, lineMinor, useAA) * fade1;
    const float s2 = LineStrength(worldXZ, levelSizes.z, lineMinor, useAA) * fade2;
    const float s3 = LineStrength(worldXZ, levelSizes.w, lineMajor, useAA) * fade3;

    const float lineMask = max(max(s0, s1), max(s2, s3));

    // Coarsest visible level sets line color — subtle stepped contrast like Blender.
    float3 color = level0Color.rgb;
    float alpha = 0.0;
    if (s3 > 1e-4)
    {
        color = level3Color.rgb;
        alpha = s3 * level3Color.a;
    }
    else if (s2 > 1e-4)
    {
        color = level2Color.rgb;
        alpha = s2 * level2Color.a;
    }
    else if (s1 > 1e-4)
    {
        color = level1Color.rgb;
        alpha = s1 * level1Color.a;
    }
    else if (s0 > 1e-4)
    {
        color = level0Color.rgb;
        alpha = s0 * level0Color.a;
    }

    if (gridFlags.y != 0)
    {
        const float axisWidth = renderParams1.y;
        const float onXAxis = AxisLineMask(worldXZ.y, abs(worldXZ.y), axisWidth, useAA);
        const float onZAxis = AxisLineMask(worldXZ.x, abs(worldXZ.x), axisWidth, useAA);

        // Render exactly one axis line per axis, no LOD stacking, no intersection alpha accumulation.
        const float xCov = saturate(onXAxis);
        const float zCov = saturate(onZAxis);
        const float axisCov = max(xCov, zCov);

        if (axisCov > 1e-5)
        {
            const bool useX = (xCov >= zCov);
            const float3 axisRgb = useX ? axisXColor.rgb : axisZColor.rgb;
            const float axisA = axisCov * (useX ? axisXColor.a : axisZColor.a);

            // Axis renders independently from grid to avoid “white” mixing with bright major lines.
            // Clamp coverage + alpha so AA can’t create a halo and RGB never exceeds configured axis color.
            color = axisRgb;
            alpha = saturate(axisA);
        }
    }

    // World-space distance fade — fully transparent beyond the configured end radius.
    const float distFadeStart = max(depthParams.z, 0.0);
    const float distFadeEnd = max(depthParams.w, distFadeStart + 1.0);
    const float distanceFade = 1.0 - smoothstep(distFadeStart, distFadeEnd, distToCamera);
    alpha = saturate(alpha * renderParams1.x * edgeFade * distanceFade);

    // Output linear HDR — global PostExposure pass applies exposure, tonemap, and sRGB once.
    float3 linearColor = WE_sRGBToLinear(saturate(color));
    const float haze = 1.0 - exp(-distToCamera * 0.0055);
    const float3 hazeColor = float3(0.02, 0.02, 0.022);
    linearColor = lerp(linearColor, hazeColor, saturate(haze * 0.35));
    // Encode linear grid colour to sRGB for the UNORM swapchain.
    // PostExposure is not in the active render graph, so we do it here.
    color = WE_LinearToSRGB(min(linearColor, float3(4.0, 4.0, 4.0)));

    if (lineMask <= 1e-5 && alpha <= 1e-5)
        discard;

    const float4 clipPos = mul(proj, mul(view, float4(fragPos.x, groundY, fragPos.z, 1.0)));
    o.depth = clipPos.z / clipPos.w;
    o.color = float4(color, alpha);
    return o;
}
