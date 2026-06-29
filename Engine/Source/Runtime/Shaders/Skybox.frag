#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform SkySettings {
    vec3 topColor;
    float gradientStrength;
    vec3 horizonColor;
    float hazeIntensity;
    vec3 groundColor;
    float exposure;
    vec3 sunDirection;
    float padding;
} sky;

float saturate(float v) {
    return clamp(v, 0.0, 1.0);
}

float hash12(vec2 p) {
    // Procedural blue-noise-like hash for anti-banding without textures.
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 toneMapACES(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    // Reconstruct a hemisphere view direction from screen UV for an infinite, seam-free sky.
    vec2 uv = inUV * 2.0 - 1.0;
    vec3 viewDir = normalize(vec3(uv.x, uv.y, 1.35));
    float upness = saturate(viewDir.y * 0.5 + 0.5);
    float horizonFactor = exp(-abs(viewDir.y) * 8.5);

    // Vertical gradient across ground/horizon/zenith with smooth interpolation.
    float gradPow = max(0.05, sky.gradientStrength);
    float zenithBlend = pow(upness, gradPow);
    float groundBlend = pow(1.0 - upness, gradPow * 0.85);
    vec3 gradient = mix(sky.groundColor, sky.topColor, zenithBlend);
    gradient = mix(gradient, sky.horizonColor, horizonFactor);
    gradient = mix(gradient, sky.groundColor, groundBlend * 0.26);

    // Physically inspired atmospheric scattering tinting.
    vec3 rayleighTint = vec3(0.42, 0.58, 1.00);
    vec3 mieTint = vec3(1.00, 0.88, 0.72);
    float rayleigh = pow(upness, 1.8) * (1.0 - horizonFactor * 0.55);
    float mie = horizonFactor * (0.65 + sky.hazeIntensity);
    vec3 atmosphere = gradient + rayleigh * 0.11 * rayleighTint + mie * 0.13 * mieTint;

    // Soft horizon haze and slight aerial perspective.
    float haze = horizonFactor * (0.24 + sky.hazeIntensity * 0.65);
    vec3 hazed = mix(atmosphere, sky.horizonColor, haze);
    float aerial = saturate(1.0 - exp(-max(viewDir.y + 0.24, 0.0) * 2.2));
    vec3 colorLinear = mix(hazed, sky.topColor, aerial * 0.12);

    // Subtle sun glow (no hard disk).
    vec3 sunDir = normalize(sky.sunDirection);
    float sunFacing = saturate(dot(viewDir, sunDir));
    float sunGlow = pow(sunFacing, 96.0) * 0.28 + pow(sunFacing, 18.0) * 0.10;
    colorLinear += vec3(1.0, 0.92, 0.78) * sunGlow * (0.60 + sky.hazeIntensity * 0.35);

    // Very subtle vignette.
    float vignette = 1.0 - saturate(dot(uv, uv) * 0.055);
    colorLinear *= mix(0.985, 1.0, vignette);

    // Exposure + HDR-friendly tonemap.
    colorLinear *= max(0.01, sky.exposure);
    vec3 mapped = toneMapACES(max(colorLinear, vec3(0.0)));

    // Gamma correction and anti-banding dithering.
    vec3 gammaColor = pow(mapped, vec3(1.0 / 2.2));
    float dither = hash12(gl_FragCoord.xy) - 0.5;
    gammaColor += dither / 255.0;
    outColor = vec4(clamp(gammaColor, 0.0, 1.0), 1.0);
}
