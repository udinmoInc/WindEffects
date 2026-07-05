#ifndef WE_ENVIRONMENT_BUFFER_HLSLI
#define WE_ENVIRONMENT_BUFFER_HLSLI

#ifndef WE_ENVIRONMENT_BUFFER_REGISTER
#define WE_ENVIRONMENT_BUFFER_REGISTER b0
#endif

cbuffer EnvironmentBuffer : register(WE_ENVIRONMENT_BUFFER_REGISTER, space0)
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
    float  sunAngularRadius;
    float  hdrSkyLuminance;
    int    sunCastShadows;
    int    sunTemperature;
    float  bloomIntensity;
    float  bloomThreshold;
    float  enableAutoExposure;
    float  postPadding;
};

#endif // WE_ENVIRONMENT_BUFFER_HLSLI
