#include "../Engine/Source/Runtime/Renderer/Public/Renderer/SceneEnvironmentUniform.hpp"
#include <cstdio>

int main() {
    printf("sizeof(SceneEnvironmentUniform) = %zu\n", sizeof(we::runtime::renderer::SceneEnvironmentUniform));
#define OFF(field) printf("  " #field " = %zu\n", offsetof(we::runtime::renderer::SceneEnvironmentUniform, field))
    OFF(sunDirection);
    OFF(sunIntensity);
    OFF(sunColor);
    OFF(skyLightIntensity);
    OFF(atmosphereRayleigh);
    OFF(mieScattering);
    OFF(ozoneAbsorption);
    OFF(planetRadius);
    OFF(eyeAltitude);
    OFF(hdrSkyLuminance);
    OFF(sunCastShadows);
    OFF(bloomIntensity);
    return 0;
}
