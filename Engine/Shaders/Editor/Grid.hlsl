#include "../Common/Camera.hlsli"
#include "../Common/Math.hlsli"

cbuffer CameraBuffer : register(b0, space0)
{
    float4x4 view;
    float4x4 proj;
    float3   cameraPos;
    float    cameraPadding;
};

cbuffer GridSettings : register(b0, space1)
{
    float fadeDistance;   // base LOD reference (world units)
    float lodIntensity;   // global grid strength multiplier
    float originWeight;   // origin-distance LOD influence
    float hdrScale;       // HDR output scale
};

struct VSOutput
{
    float4 position  : SV_Position;
    float3 nearPoint : TEXCOORD0;
    float3 farPoint  : TEXCOORD1;
};

static const float3 kGridPositions[6] = {
    float3(-1, -1, 0), float3(1, -1, 0), float3(1, 1, 0),
    float3(-1, -1, 0), float3(1,  1, 0), float3(-1, 1, 0)
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    VSOutput o;
    float3 p = kGridPositions[vertexId];
    o.nearPoint = WE_UnprojectPoint(p.x, p.y, 0.0, view, proj);
    o.farPoint  = WE_UnprojectPoint(p.x, p.y, 1.0, view, proj);
    o.position = float4(p, 1.0);
    return o;
}

// Logarithmic fade — smooth across orders of magnitude, no LOD pops.
float WE_LogFade(float dist, float fadeStart, float fadeEnd)
{
    float logD = log(max(dist, 0.05));
    float logA = log(max(fadeStart, 0.1));
    float logB = log(max(fadeEnd, fadeStart + 0.1));
    return 1.0 - smoothstep(logA, logB, logD);
}

// Exponential distance falloff for fine control near thresholds.
float WE_ExpRangeFade(float dist, float range, float power)
{
    return exp(-pow(saturate(dist / max(range, 0.01)), power));
}

// Procedural anti-aliased grid line (no textures). Returns line coverage [0,1].
float WE_GridLineAA(float2 worldXZ, float cellSize, float thicknessScale)
{
    float2 coord = worldXZ / cellSize;
    float2 fw = max(fwidth(coord) * thicknessScale, float2(1e-5, 1e-5));
    float2 cellLine = abs(frac(coord - 0.5) - 0.5);
    float2 lines = cellLine / fw;
    return 1.0 - saturate(min(lines.x, lines.y));
}

// Anti-aliased world-space axis line.
float WE_AxisLineAA(float distToAxis, float thicknessScale)
{
    float fw = max(fwidth(distToAxis) * thicknessScale, 1e-5);
    return 1.0 - saturate(abs(distToAxis) / fw);
}

struct PSOutput
{
    float4 color : SV_Target0;
    float  depth : SV_Depth;
};

PSOutput PSMain(VSOutput input)
{
    PSOutput o;
    if (abs(input.farPoint.y - input.nearPoint.y) < 1e-6)
        discard;

    float t = -input.nearPoint.y / (input.farPoint.y - input.nearPoint.y);
    if (t < 0.0)
        discard;

    float3 fragPos3D = input.nearPoint + t * (input.farPoint - input.nearPoint);

    float4 clipSpacePos = mul(proj, mul(view, float4(fragPos3D, 1.0)));
    o.depth = clipSpacePos.z / clipSpacePos.w;

    // Camera / viewport metrics for zoom-aware LOD.
    float zoomFactor = 2.0 / max(abs(proj[1][1]), 1e-4);
    float distCam = length(fragPos3D - cameraPos);
    float distOrigin = length(fragPos3D.xz);
    float camOriginDist = length(cameraPos.xz);

    float lodRef = max(fadeDistance, 10.0) * zoomFactor;
    float lodMetric = distCam * lerp(1.0, distOrigin / max(camOriginDist, 1.0), saturate(originWeight));

    // Auto line thickness — wider when zoomed out, thinner when zoomed in.
    float thicknessScale = clamp(zoomFactor * 0.65, 0.6, 2.8);

    // Per-level LOD fades (logarithmic ranges). Fine fades first, coarse last.
    float fineFade   = WE_LogFade(lodMetric, lodRef * 0.06, lodRef * 0.28);
    float mediumFade = WE_LogFade(lodMetric, lodRef * 0.20, lodRef * 0.62);
    float coarseFade = WE_LogFade(lodMetric, lodRef * 0.48, lodRef * 1.05);

    // Axes fade independently and persist longer than grid lines.
    float axisXFade  = WE_LogFade(lodMetric, lodRef * 0.55, lodRef * 1.80);
    float axisYFade  = WE_LogFade(lodMetric, lodRef * 0.70, lodRef * 2.40);
    float axisZFade  = WE_LogFade(lodMetric, lodRef * 0.60, lodRef * 2.00);

    // Global fade — grid nearly disappears when extremely far / zoomed out.
    float globalFade = WE_LogFade(lodMetric, lodRef * 1.10, lodRef * 2.80);
    globalFade *= WE_ExpRangeFade(distOrigin, lodRef * 3.5, 1.6);

    float intensity = max(lodIntensity, 0.0);
    float hdr = max(hdrScale, 0.01);

    float2 worldXZ = fragPos3D.xz;

    // Three procedural LOD scales (UE5-style 1 / 10 / 100 unit hierarchy).
    float fineLine   = WE_GridLineAA(worldXZ, 1.0,   thicknessScale) * fineFade;
    float mediumLine = WE_GridLineAA(worldXZ, 10.0,  thicknessScale) * mediumFade;
    float coarseLine = WE_GridLineAA(worldXZ, 100.0, thicknessScale) * coarseFade;

    float3 minorColor = float3(0.18, 0.18, 0.18);
    float3 majorColor = float3(0.24, 0.24, 0.24);

    float gridAlpha = saturate(fineLine * 0.55 + mediumLine * 0.35 + coarseLine * 0.22);
    float3 gridRgb = lerp(minorColor, majorColor, saturate(coarseLine)) * hdr;

    // Independent axis lines (HDR linear; axes tinted, fade separately).
    float axisXLine = WE_AxisLineAA(fragPos3D.z, thicknessScale * 1.15);
    float axisZLine = WE_AxisLineAA(fragPos3D.x, thicknessScale * 1.15);
    float axisYLine = WE_AxisLineAA(length(fragPos3D.xz), thicknessScale * 0.85);

    float3 axisXColor = float3(0.55, 0.18, 0.16) * hdr;
    float3 axisYColor = float3(0.18, 0.52, 0.20) * hdr;
    float3 axisZColor = float3(0.16, 0.22, 0.52) * hdr;

    float fade = globalFade * intensity;

    float3 finalColor = gridRgb;
    float finalAlpha = gridAlpha * fade;

    float axX = axisXLine * axisXFade * fade;
    if (axX > finalAlpha)
    {
        finalColor = axisXColor;
        finalAlpha = axX;
    }

    float axZ = axisZLine * axisZFade * fade;
    if (axZ > finalAlpha)
    {
        finalColor = axisZColor;
        finalAlpha = axZ;
    }

    float axY = axisYLine * axisYFade * fade;
    if (axY > finalAlpha)
    {
        finalColor = axisYColor;
        finalAlpha = axY;
    }

    if (finalAlpha < 0.004)
        discard;

    o.color = float4(finalColor, finalAlpha);
    return o;
}
