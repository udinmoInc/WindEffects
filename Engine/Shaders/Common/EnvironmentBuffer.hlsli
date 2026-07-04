#ifndef WE_ENVIRONMENT_BUFFER_HLSLI
#define WE_ENVIRONMENT_BUFFER_HLSLI

// Shared std140 environment uniform for all atmosphere / cloud / fog / lighting passes.
cbuffer EnvironmentBuffer : register(b0, space0)
{
    float3 sunDirection;
    float  sunIntensity;
    float3 sunColor;
    float  skyLightIntensity;
    float3 skyAmbientColor;
    float  fogDensity;
    float3 skyLightLowerColor;
    float  fogHeightFalloff;
    float3 fogColor;
    float  fogStartDistance;
    float3 atmosphereRayleigh;
    float  mieScattering;
    float3 ozoneAbsorption;
    float  mieAnisotropy;
    float3 worldOrigin;
    float  exposureEV;
    float  planetRadius;
    float  atmosphereHeight;
    float  multiScatterStrength;
    float  eyeAltitude;
    float  cloudCoverage;
    float  cloudAltitude;
    float  cloudExtinction;
    float  enableClouds;
    float3 cloudColor;
    float  enableVolumetricFog;
    float  exposureCompensation;
    float  padding0;
    float  padding1;
    int    sunCastShadows;
    int    sunTemperature;
    int2   envPadding;
};

#endif // WE_ENVIRONMENT_BUFFER_HLSLI
