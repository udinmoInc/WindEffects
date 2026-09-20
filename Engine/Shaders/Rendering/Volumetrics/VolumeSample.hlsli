#ifndef WE_VOLUME_SAMPLE_HLSLI
#define WE_VOLUME_SAMPLE_HLSLI

// Common volumetric sample — all providers emit this shape.

struct VolumeSample
{
    float density;
    float extinction;
    float scattering;
    float anisotropy;
};

VolumeSample MakeVolumeSample(float density, float extinction, float scattering, float anisotropy)
{
    VolumeSample s;
    s.density = density;
    s.extinction = extinction;
    s.scattering = scattering;
    s.anisotropy = anisotropy;
    return s;
}

VolumeSample EmptyVolumeSample()
{
    return MakeVolumeSample(0.0, 0.0, 0.0, 0.0);
}

VolumeSample AccumulateVolumeSample(VolumeSample a, VolumeSample b)
{
    VolumeSample s;
    s.density = a.density + b.density;
    s.extinction = a.extinction + b.extinction;
    s.scattering = a.scattering + b.scattering;
    // Density-weighted anisotropy blend.
    const float w = max(a.density + b.density, 1e-6);
    s.anisotropy = (a.anisotropy * a.density + b.anisotropy * b.density) / w;
    return s;
}

#endif // WE_VOLUME_SAMPLE_HLSLI
