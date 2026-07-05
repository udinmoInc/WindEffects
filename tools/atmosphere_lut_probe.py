#!/usr/bin/env python3
"""Probe atmosphere LUT averages and channel bias (matches AtmosphereLUTGenerator.cpp)."""

import math

K_PI = math.pi
RAYLEIGH_SCALE_KM = 8.0
MIE_SCALE_KM = 1.2
OZONE_PEAK_ALT_KM = 25.0
OZONE_WIDTH_KM = 8.0
ATMOSPHERE_STEPS = 16
SUN_TRANSMITTANCE_STEPS = 8

DEFAULT_RAYLEIGH = (0.005802, 0.013558, 0.033100)
DEFAULT_OZONE = (0.00065, 0.0018, 0.00008)


def vadd(a, b):
    return tuple(x + y for x, y in zip(a, b))


def vmul(a, s):
    return tuple(x * s for x in a)


def vmul3(a, b):
    return tuple(x * y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def length(a):
    return math.sqrt(dot(a, a))


def normalize(a):
    l = length(a)
    return tuple(x / l for x in a)


def atmosphere_density(height_km, scale_km):
    return math.exp(-max(height_km, 0.0) / max(scale_km, 1e-3))


def ozone_density(height_km):
    x = (height_km - OZONE_PEAK_ALT_KM) / OZONE_WIDTH_KM
    return math.exp(-x * x)


def rayleigh_phase(cos_theta):
    return (3.0 / (16.0 * K_PI)) * (1.0 + cos_theta * cos_theta)


def mie_phase(cos_theta, g):
    g2 = g * g
    num = 1.0 - g2
    denom = max(1.0 + g2 - 2.0 * g * cos_theta, 1e-4) ** 1.5
    return (3.0 / (8.0 * K_PI)) * ((1.0 + g2) * num / denom)


def intersect_sphere(origin, direction, radius):
    b = dot(origin, direction)
    c = dot(origin, origin) - radius * radius
    discriminant = b * b - c
    if discriminant < 0.0:
        return False, 0.0, 0.0
    s = math.sqrt(discriminant)
    return True, -b - s, -b + s


def integrate_optical_depth(origin, direction, max_dist, rayleigh, mie, ozone):
    rayleigh_depth = mie_depth = ozone_depth = (0.0, 0.0, 0.0)
    step = max_dist / SUN_TRANSMITTANCE_STEPS
    pos = origin
    for _ in range(SUN_TRANSMITTANCE_STEPS):
        height = max(length(pos) - planet_radius, 0.0)
        rayleigh_depth = vadd(rayleigh_depth, vmul(rayleigh, atmosphere_density(height, RAYLEIGH_SCALE_KM) * step))
        mie_depth = vadd(mie_depth, vmul((mie, mie, mie), atmosphere_density(height, MIE_SCALE_KM) * step))
        ozone_depth = vadd(ozone_depth, vmul(ozone, ozone_density(height) * step))
        pos = vadd(pos, vmul(direction, step))
    tau = vadd(vadd(rayleigh_depth, mie_depth), ozone_depth)
    return tuple(math.exp(-t) for t in tau)


def integrate_inscattering(view_dir, sun_dir, origin, rayleigh, mie, ozone, mie_g, sun_color, sun_intensity):
    view_dir = normalize(view_dir)
    sun_dir = normalize(sun_dir)
    ok, t0, t1 = intersect_sphere(origin, view_dir, atmosphere_radius)
    if not ok:
        return (0.0, 0.0, 0.0)

    start = max(t0, 0.0)
    end = t1
    step = (end - start) / ATMOSPHERE_STEPS

    sum_rayleigh = (0.0, 0.0, 0.0)
    sum_mie = (0.0, 0.0, 0.0)
    optical_rayleigh = (0.0, 0.0, 0.0)
    optical_mie = (0.0, 0.0, 0.0)
    optical_ozone = (0.0, 0.0, 0.0)

    cos_theta = dot(view_dir, sun_dir)
    phase_r = rayleigh_phase(cos_theta)
    phase_m = mie_phase(cos_theta, mie_g)

    for i in range(ATMOSPHERE_STEPS):
        t = start + (i + 0.5) * step
        sample = vadd(origin, vmul(view_dir, t))
        height = max(length(sample) - planet_radius, 0.0)
        rd = atmosphere_density(height, RAYLEIGH_SCALE_KM)
        md = atmosphere_density(height, MIE_SCALE_KM)
        od = ozone_density(height)

        optical_rayleigh = vadd(optical_rayleigh, vmul(rayleigh, rd * step))
        optical_mie = vadd(optical_mie, vmul((mie, mie, mie), md * step))
        optical_ozone = vadd(optical_ozone, vmul(ozone, od * step))

        dist = atmosphere_radius - length(sample)
        sun_t = integrate_optical_depth(sample, sun_dir, max(dist, 0.001), rayleigh, mie, ozone)
        tau = vadd(vadd(optical_rayleigh, optical_mie), optical_ozone)
        atten = vmul3(tuple(math.exp(-t) for t in tau), sun_t)
        sum_rayleigh = vadd(sum_rayleigh, vmul(atten, rd * step))
        sum_mie = vadd(sum_mie, vmul(atten, md * step))

    multi = vmul3(
        tuple(1.0 - math.exp(-t) for t in vmul(vadd(optical_rayleigh, optical_mie), 0.12)),
        rayleigh,
    )
    sun_radiance = vmul(sun_color, sun_intensity)
    return vmul3(
        sun_radiance,
        vadd(
            vadd(vmul3(sum_rayleigh, vmul(rayleigh, phase_r)), vmul3(sum_mie, vmul((mie, mie, mie), phase_m))),
            multi,
        ),
    )


def average_lut(name, width, height, generator):
    r_sum = g_sum = b_sum = 0.0
    count = 0
    for y in range(height):
        for x in range(width):
            rgb = generator(x, y, width, height)
            r_sum += rgb[0]
            g_sum += rgb[1]
            b_sum += rgb[2]
            count += 1
    avg = (r_sum / count, g_sum / count, b_sum / count)
    print(
        f"{name:16} avg RGB=({avg[0]:.6f}, {avg[1]:.6f}, {avg[2]:.6f}) "
        f"G/R={avg[1]/max(avg[0],1e-6):.3f} B/G={avg[2]/max(avg[1],1e-6):.3f} B/R={avg[2]/max(avg[0],1e-6):.3f}"
    )
    return avg


def configure_defaults(pitch_deg=-45.0):
    global planet_radius, atmosphere_radius, sun_dir, sky_origin, rayleigh, mie, ozone, mie_g, sun_color, sun_intensity

    planet_radius = 6360.0
    atmosphere_radius = planet_radius + 60.0
    mie = 0.003996
    ozone = DEFAULT_OZONE
    mie_g = 0.76
    sun_color = (0.997, 0.997, 0.98)
    sun_intensity = 10.0
    rayleigh = DEFAULT_RAYLEIGH

    # Rotation pitch follows editor convention: negative pitch = sun above horizon.
    pitch = math.radians(pitch_deg)
    yaw = math.radians(35.0)
    light_travel = (
        math.cos(pitch) * math.sin(yaw),
        math.sin(pitch),
        math.cos(pitch) * math.cos(yaw),
    )
    light_travel = normalize(light_travel)
    sun_dir = tuple(-x for x in light_travel)
    sky_origin = (0.0, planet_radius + 0.001, 0.0)


def main():
    configure_defaults()

    def transmittance(x, y, w, h):
        h_norm = (x + 0.5) / w
        mu_norm = (y + 0.5) / h
        height = h_norm * (atmosphere_radius - planet_radius)
        cos_zenith = mu_norm * 2.0 - 1.0
        origin = (0.0, planet_radius + height, 0.0)
        view = (0.0, cos_zenith, math.sqrt(max(1.0 - cos_zenith * cos_zenith, 0.0)))
        dist = atmosphere_radius - length(origin)
        return integrate_optical_depth(origin, view, max(dist, 0.001), rayleigh, mie, ozone)

    def multi_scatter(x, y, w, h):
        cos_sun = (x + 0.5) / w * 2.0 - 1.0
        h_norm = (y + 0.5) / h
        height = h_norm * (atmosphere_radius - planet_radius)
        origin = (0.0, planet_radius + height, 0.0)
        _, rd, md = (0, (0, 0, 0), (0, 0, 0))
        step = atmosphere_radius / SUN_TRANSMITTANCE_STEPS
        pos = origin
        rd = md = od = (0.0, 0.0, 0.0)
        for _ in range(SUN_TRANSMITTANCE_STEPS):
            height_km = max(length(pos) - planet_radius, 0.0)
            rd = vadd(rd, vmul(rayleigh, atmosphere_density(height_km, RAYLEIGH_SCALE_KM) * step))
            md = vadd(md, vmul((mie, mie, mie), atmosphere_density(height_km, MIE_SCALE_KM) * step))
            pos = vadd(pos, (0.0, step, 0.0))
        multi = vmul3(tuple(1.0 - math.exp(-t) for t in vmul(vadd(rd, md), 0.15)), rayleigh)
        return vmul(multi, 0.5 + cos_sun * 0.5)

    def sky_view(x, y, w, h):
        azimuth = (x + 0.5) / w
        view_zenith = (y + 0.5) / h
        theta = view_zenith * K_PI
        phi = azimuth * 2.0 * K_PI
        view = (math.sin(theta) * math.cos(phi), math.cos(theta), math.sin(theta) * math.sin(phi))
        sky = integrate_inscattering(view, sun_dir, sky_origin, rayleigh, mie, ozone, mie_g, sun_color, sun_intensity)
        return tuple(max(0.0, c) for c in sky)

    def aerial(x, y, w, h):
        dist_km = (x + 0.5) / w * 64.0
        height_km = (y + 0.5) / h * (atmosphere_radius - planet_radius)
        view = normalize((0.3, 0.15, 1.0))
        cam = (0.0, 0.001 * 1000.0, 0.0)
        surface = (0.0, height_km * 1000.0, dist_km * 1000.0)
        origin = (0.0, planet_radius + 0.001, 0.0)
        march = normalize((surface[0] - cam[0], surface[1] - cam[1], surface[2] - cam[2]))
        inscatter = integrate_inscattering(march, sun_dir, origin, rayleigh, mie, ozone, mie_g, sun_color, sun_intensity)
        fade = 1.0 - math.exp(-dist_km * 0.06)
        return vmul(inscatter, fade)

    print("=== Atmosphere LUT averages (default Sunny preset) ===")
    average_lut("Transmittance", 256, 64, transmittance)
    average_lut("MultiScatter", 32, 32, multi_scatter)
    average_lut("SkyView", 192, 96, sky_view)
    average_lut("AerialPerspective", 32, 32, aerial)

    print("\n=== Directional reference samples ===")
    for label, view in [("Zenith", (0, 1, 0)), ("Horizon", normalize((1, 0.01, 0)))]:
        sky = integrate_inscattering(view, sun_dir, sky_origin, rayleigh, mie, ozone, mie_g, sun_color, sun_intensity)
        print(
            f"{label:8} RGB=({sky[0]:.4f},{sky[1]:.4f},{sky[2]:.4f}) "
            f"G/R={sky[1]/max(sky[0],1e-6):.2f} B/G={sky[2]/max(sky[1],1e-6):.2f}"
        )

    print("\n=== Channel-order sensitivity (zenith) ===")
    for label, coeffs in [
        ("Correct RGB", DEFAULT_RAYLEIGH),
        ("Swapped G/B", (0.005802, 0.033100, 0.013558)),
        ("Reversed RGB", (0.033100, 0.013558, 0.005802)),
    ]:
        sky = integrate_inscattering(
            (0, 1, 0), sun_dir, sky_origin, coeffs, mie, ozone, mie_g, sun_color, sun_intensity
        )
        print(
            f"{label:12} RGB=({sky[0]:.4f},{sky[1]:.4f},{sky[2]:.4f}) "
            f"G/R={sky[1]/max(sky[0],1e-6):.2f} B/G={sky[2]/max(sky[1],1e-6):.2f}"
        )


if __name__ == "__main__":
    main()
