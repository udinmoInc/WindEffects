#include "../Common/Camera.hlsli"
#include "../Common/Math.hlsli"

cbuffer CameraBuffer : register(b0, space0)
{
    float4x4 view;
    float4x4 proj;
    float3   cameraPos;
    float    cameraPadding;
};

// Spacing is chosen once per frame on the CPU to keep ~20–40 cells across the viewport.
cbuffer GridSettings : register(b0, space1)
{
    float cellSize;
    float majorCellSize;
    float hdrScale;
    float fadeDistance;
    float lodIntensity;
    float thicknessScale;
    float2 padding;
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

float WE_HorizonFade(float groundDist, float refDist)
{
    float fadeStart = refDist * 0.80;
    float fadeEnd   = refDist * 1.02;
    return 1.0 - smoothstep(fadeStart, fadeEnd, groundDist);
}

float WE_LineAA(float2 worldXZ, float spacing, float thicknessScale)
{
    float2 coord = worldXZ / max(spacing, 1e-4);
    float2 fw    = max(fwidth(coord) * thicknessScale, float2(1e-4, 1e-4));
    float2 dist  = abs(frac(coord - 0.5) - 0.5);
    float lineX  = 1.0 - saturate(dist.x / fw.x);
    float lineZ  = 1.0 - saturate(dist.y / fw.y);
    return max(lineX, lineZ);
}

float WE_AxisLineAA(float distToAxis, float thicknessScale)
{
    float fw = max(fwidth(distToAxis) * thicknessScale, 1e-4);
    return 1.0 - saturate(abs(distToAxis) / fw);
}

float WE_RenderActiveGrid(float2 worldXZ, float spacing, float majorSpacing, float thicknessScale)
{
    float primary = WE_LineAA(worldXZ, spacing, thicknessScale) * 0.24;

    if (majorSpacing > spacing + 1e-3)
    {
        float major = WE_LineAA(worldXZ, majorSpacing, thicknessScale) * 0.36;
        return max(primary, major);
    }
    return primary;
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

    float groundDist = length(fragPos3D.xz - cameraPos.xz);
    float maxDist    = max(fadeDistance, 1.0);

    if (groundDist > maxDist)
        discard;

    // Snap to camera-relative coords for stable lines and better precision at large world positions.
    float snap = max(cellSize, 1e-4);
    float2 gridOrigin = floor(cameraPos.xz / snap) * snap;
    float2 worldXZ = fragPos3D.xz - gridOrigin;
    float gridLine = WE_RenderActiveGrid(worldXZ, cellSize, majorCellSize, thicknessScale);

    float3 gridColor = float3(0.108, 0.110, 0.114);

    float intensity   = max(lodIntensity, 0.0);
    float hdr         = max(hdrScale, 0.01);
    float horizonFade = WE_HorizonFade(groundDist, maxDist);
    float fade        = horizonFade * intensity;

    float axisXLine = WE_AxisLineAA(fragPos3D.z, thicknessScale);
    float axisZLine = WE_AxisLineAA(fragPos3D.x, thicknessScale);

    float3 axisXColor = float3(0.58, 0.20, 0.17) * hdr;
    float3 axisZColor = float3(0.17, 0.34, 0.72) * hdr;
    float3 axisYColor = float3(0.20, 0.58, 0.22) * hdr;

    float3 finalColor = gridColor * hdr;
    float finalAlpha  = gridLine * fade;

    float axX = axisXLine * fade * 0.85;
    if (axX > finalAlpha) { finalColor = axisXColor; finalAlpha = axX; }

    float axZ = axisZLine * fade;
    if (axZ > finalAlpha) { finalColor = axisZColor; finalAlpha = axZ; }

    float yMark = WE_AxisLineAA(length(fragPos3D.xz), thicknessScale);
    yMark *= exp(-dot(fragPos3D.xz, fragPos3D.xz) / 0.25);
    float axY = yMark * fade * 0.70;
    if (axY > finalAlpha) { finalColor = axisYColor; finalAlpha = axY; }

    if (finalAlpha < 0.002)
        discard;

    o.color = float4(finalColor, finalAlpha);
    return o;
}
