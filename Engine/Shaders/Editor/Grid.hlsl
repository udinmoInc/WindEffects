#include "../Common/Camera.hlsli"
#include "../Common/Math.hlsli"

cbuffer CameraBuffer : register(b0, space0)
{
    float4x4 view;
    float4x4 proj;
    float3   cameraPos;
    float    cameraPadding;
};

// Grid LOD is chosen once per frame on the CPU — every pixel uses the same spacing.
cbuffer GridSettings : register(b0, space1)
{
    float cellSizeA;
    float cellSizeB;
    float lodBlend;
    float hdrScale;
    float fadeDistance;
    float lodIntensity;
    float originSnap;
    float thicknessScale;
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
    float fadeStart = refDist * 0.55;
    float fadeEnd   = refDist * 2.6;
    float logD = log(max(groundDist, 0.5));
    float logA = log(max(fadeStart, 1.0));
    float logB = log(max(fadeEnd, fadeStart + 1.0));
    return 1.0 - smoothstep(logA, logB, logD);
}

float2 WE_CameraRelativeXZ(float2 worldXZ, float snapCell)
{
    float snap = max(snapCell, 1e-4);
    float2 origin = floor(cameraPos.xz / snap) * snap;
    return worldXZ - origin;
}

float WE_LineAA(float2 relXZ, float cellSize, float thicknessScale)
{
    float2 coord = relXZ / max(cellSize, 1e-4);
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

float WE_RenderGridTier(float2 relXZ, float cellSize, float thicknessScale)
{
    float primary = WE_LineAA(relXZ, cellSize, thicknessScale) * 0.26;

    float majorSize = cellSize * 10.0;
    float major = WE_LineAA(relXZ, majorSize, thicknessScale * 1.12) * 0.48;

    return max(primary, major);
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
    float lodRef     = max(fadeDistance, 200.0);

    float2 worldXZ = fragPos3D.xz;
    float2 relXZ   = WE_CameraRelativeXZ(worldXZ, originSnap);

    float gridA = WE_RenderGridTier(relXZ, cellSizeA, thicknessScale);
    float gridB = WE_RenderGridTier(relXZ, cellSizeB, thicknessScale);
    float gridLine = lerp(gridA, gridB, lodBlend);

    float3 lineColor  = float3(0.092, 0.094, 0.097);
    float3 majorColor = float3(0.125, 0.129, 0.135);
    float3 gridColor  = lerp(lineColor, majorColor, saturate(gridLine * 2.8));

    float intensity = max(lodIntensity, 0.0);
    float hdr       = max(hdrScale, 0.01);

    float horizonFade = WE_HorizonFade(groundDist, lodRef);
    float fade        = horizonFade * intensity;

    float axisXLine = WE_AxisLineAA(fragPos3D.z, thicknessScale * 1.1);
    float axisZLine = WE_AxisLineAA(fragPos3D.x, thicknessScale * 1.1);

    float3 axisXColor = float3(0.58, 0.20, 0.17) * hdr;
    float3 axisZColor = float3(0.17, 0.34, 0.72) * hdr;
    float3 axisYColor = float3(0.20, 0.58, 0.22) * hdr;

    float3 finalColor = gridColor * hdr;
    float finalAlpha  = gridLine * fade;

    float axX = axisXLine * fade * 0.85;
    if (axX > finalAlpha) { finalColor = axisXColor; finalAlpha = axX; }

    float axZ = axisZLine * fade;
    if (axZ > finalAlpha) { finalColor = axisZColor; finalAlpha = axZ; }

    float yMark = WE_AxisLineAA(length(fragPos3D.xz), thicknessScale * 2.5);
    yMark *= exp(-dot(fragPos3D.xz, fragPos3D.xz) / 0.25);
    float axY = yMark * fade * 0.70;
    if (axY > finalAlpha) { finalColor = axisYColor; finalAlpha = axY; }

    if (finalAlpha < 0.002)
        discard;

    o.color = float4(finalColor, finalAlpha);
    return o;
}
