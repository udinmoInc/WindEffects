#include "../Common/Camera.hlsli"
#include "../Common/Color.hlsli"

#define WE_ENVIRONMENT_BUFFER_REGISTER b2
#include "../Common/EnvironmentBuffer.hlsli"
#include "../Rendering/SkyAtmosphere.hlsli"

struct VSInput
{
    float3 position : POSITION0;
    float3 normal   : NORMAL0;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position   : SV_Position;
    float3 worldPos   : TEXCOORD0;
    float3 worldNormal: TEXCOORD1;
    float2 texCoord   : TEXCOORD2;
};

cbuffer CameraBuffer : register(b0, space0)
{
    float4x4 view;
    float4x4 proj;
    float3   cameraPos;
    float    cameraPadding;
};

cbuffer ObjectBuffer : register(b1, space0)
{
    float4x4 model;
    float4   color;
    int      mode;
    int3     objectPadding;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    float4 worldPos = mul(model, float4(input.position, 1.0));
    o.worldPos = worldPos.xyz;
    o.worldNormal = normalize(mul(model, float4(input.normal, 0.0)).xyz);
    o.texCoord = input.texCoord;
    o.position = mul(proj, mul(view, worldPos));
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    const float3 albedo = WE_sRGBToLinear(saturate(color.rgb));

    if (mode == 1 || mode == 2)
    {
        return float4(albedo, color.a);
    }

    float3 normal = normalize(input.worldNormal);
    float3 lightDir = normalize(sunDirection);
    const float3 relPos = input.worldPos - worldOrigin;
    const float3 relCam = cameraPos - worldOrigin;
    float3 viewDir = normalize(relCam - relPos);

    const float3 sunLinear = max(sunColor, float3(0.0, 0.0, 0.0));
    const float3 skyUpper = max(skyAmbientColor, float3(0.0, 0.0, 0.0));
    const float3 skyLower = max(skyLightLowerColor, float3(0.0, 0.0, 0.0));
    const float upN = saturate(normal.y * 0.5 + 0.5);
    const float3 ambient = lerp(skyLower * 0.05, skyUpper * 0.15, upN) * skyLightIntensity;

    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * sunLinear * (sunIntensity * 0.011);

    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
    float3 specular = 0.045 * spec * sunLinear * skyLightIntensity;

    float3 litLinear = albedo * (ambient + diffuse) + specular;

    // Aerial perspective toward procedurally matched horizon inscattering.
    const float dist = length(relCam - relPos);
    const float haze = 1.0 - exp(-dist * 0.0025);
    const float3 rayleigh = max(atmosphereRayleigh, float3(1e-6, 1e-6, 1e-6));
    const float3 ozone = max(ozoneAbsorption, float3(0.0, 0.0, 0.0));
    const float3 horizonInscatter = WE_SampleSkyAtmosphere(
        normalize(lerp(viewDir, float3(viewDir.x, 0.15, viewDir.z), 0.5)),
        sunDirection, cameraPos, worldOrigin,
        sunLinear, sunIntensity * 0.35,
        rayleigh, mieScattering, ozone, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
        max(sunAngularRadius, WE_SUN_ANGULAR_RADIUS));
    litLinear = lerp(litLinear, horizonInscatter, saturate(haze * 0.45));

    return float4(litLinear, color.a);
}
