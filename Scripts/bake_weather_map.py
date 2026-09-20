#!/usr/bin/env python3
# ==============================================================================
# WindEffects — Horizon Zero Dawn / Nubis 2D Weather Map Baker
#
# Procedurally generates CloudWeatherMap1024.dds — the Schneider (SIGGRAPH 2015 /
# GPU Pro 7) weather map. This is a 2D world-space WEATHER FIELD only:
#
#   R — Coverage scalar   (amount of 3D Texture1 density that survives)
#   G — Precipitation     (storm / dark underbelly; shader precip→CB)
#   B — Cloud type        (0 = stratus … 0.5 = cumulus … 1 = cumulonimbus)
#   A — Neutral / unused as morphology
#
# WeatherMap must NOT generate cloud islands, footprints, silhouettes, spacing,
# clustering, or 3D formations. Those come from Texture1 (+ height profiles).
# B is authored independently of R (type must not simply repeat coverage).
#
# Usage:
#   python bake_weather_map.py --outdir Engine/EngineContent/Cloud
#   python bake_weather_map.py --outdir <path> --size 1024 --seed 2026
# ==============================================================================

from __future__ import annotations

import argparse
import logging
import struct
import sys
import time
from dataclasses import dataclass
from pathlib import Path

import numpy as np

LOG = logging.getLogger("weather_map_baker")

DEFAULT_FILENAME = "CloudWeatherMap1024.dds"
DEFAULT_SIZE = 1024


def _configure_logging(verbose: bool) -> None:
    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(
        level=level,
        format="[%(asctime)s] %(levelname)s  %(message)s",
        datefmt="%H:%M:%S",
    )


# ===========================================================================
# Schneider remap
# ===========================================================================

def remap(
    val: np.ndarray | float,
    orig_min: float,
    orig_max: float,
    new_min: np.ndarray | float,
    new_max: float,
) -> np.ndarray | float:
    return new_min + ((val - orig_min) / max(orig_max - orig_min, 1e-8)) * (new_max - new_min)


def remap_clamped(
    val: np.ndarray,
    orig_min: float,
    orig_max: float,
    new_min: float = 0.0,
    new_max: float = 1.0,
) -> np.ndarray:
    return np.clip(remap(val, orig_min, orig_max, new_min, new_max), new_min, new_max)


# ===========================================================================
# Periodic hash / fade / gradients
# ===========================================================================

def _hash2(ix: np.ndarray, iy: np.ndarray, seed: int = 1337) -> np.ndarray:
    n = (
        ix.astype(np.int64) * 73856093
        ^ iy.astype(np.int64) * 19349663
        ^ int(seed)
    )
    n = (n * 2654435761) & 0xFFFFFFFF
    return n.astype(np.float64) / 4294967296.0


def _fade(t: np.ndarray) -> np.ndarray:
    """Perlin quintic: 6t^5 - 15t^4 + 10t^3."""
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0)


# ===========================================================================
# Seamless 2D Perlin / FBM  (period = frequency cells across the map)
# ===========================================================================

def perlin2d_periodic(
    coords: np.ndarray,
    frequency: int,
    seed: int = 1337,
) -> np.ndarray:
    """Periodic 2D classic Perlin. coords (..., 2) in [0, 1). Returns ~[-1, 1]."""
    if frequency < 1:
        raise ValueError("frequency must be >= 1")

    grads = np.array(
        [
            [1.0, 0.0], [-1.0, 0.0], [0.0, 1.0], [0.0, -1.0],
            [0.7071, 0.7071], [-0.7071, 0.7071],
            [0.7071, -0.7071], [-0.7071, -0.7071],
        ],
        dtype=np.float64,
    )

    p = coords * float(frequency)
    cell = np.floor(p).astype(np.int64)
    f = p - cell.astype(np.float64)
    u = _fade(f)

    def corner(dx: int, dy: int) -> np.ndarray:
        ix = (cell[..., 0] + dx) % frequency
        iy = (cell[..., 1] + dy) % frequency
        h = _hash2(ix, iy, seed=seed)
        gi = (h * 8.0).astype(np.int64) % 8
        g = grads[gi]
        offset = f - np.array([dx, dy], dtype=np.float64)
        return np.sum(g * offset, axis=-1)

    n00 = corner(0, 0)
    n10 = corner(1, 0)
    n01 = corner(0, 1)
    n11 = corner(1, 1)
    nx0 = n00 + u[..., 0] * (n10 - n00)
    nx1 = n01 + u[..., 0] * (n11 - n01)
    return nx0 + u[..., 1] * (nx1 - nx0)


def perlin_fbm2d01(
    coords: np.ndarray,
    base_frequency: int = 2,
    octaves: int = 5,
    lacunarity: float = 2.0,
    gain: float = 0.5,
    seed: int = 1337,
) -> np.ndarray:
    """Periodic Perlin FBM remapped to [0, 1]. Frequency stays integer for tiling."""
    amp = 1.0
    amp_sum = 0.0
    total = np.zeros(coords.shape[:-1], dtype=np.float64)
    freq = float(base_frequency)

    for o in range(octaves):
        f_int = max(1, int(round(freq)))
        n = perlin2d_periodic(coords, frequency=f_int, seed=seed + o * 97)
        total += n * amp
        amp_sum += amp
        amp *= gain
        freq *= lacunarity

    total = total / max(amp_sum, 1e-8)
    return np.clip(total * 0.5 + 0.5, 0.0, 1.0)


# ===========================================================================
# Weather map synthesis (HZD channel layout — weather field only)
# ===========================================================================

def _make_coords(size: int) -> np.ndarray:
    """Texel-center UVs in [0, 1)^2, shape (H, W, 2)."""
    t = (np.arange(size, dtype=np.float64) + 0.5) / size
    xx, yy = np.meshgrid(t, t, indexing="xy")
    return np.stack([xx, yy], axis=-1)


def generate_weather_map(
    size: int = DEFAULT_SIZE,
    seed: int = 2026,
    *,
    # Map assumed ~120 km across → freq 2–4 ≈ 30–60 km features.
    coverage_freq: int = 2,
    coverage_octaves: int = 5,
    precip_strength: float = 0.85,
) -> np.ndarray:
    """Build float32 RGBA weather map in [0, 1], shape (H, W, 4).

    HZD / Nubis weather-control roles only (NOT a formation / island field):
      R — coverage scalar (amount of 3D Texture1 density that survives)
      G — precipitation (storm / precip→CB tendency in the shader)
      B — cloud type continuum (St→Sc→Cu→Cb height-profile blend)
      A — neutral / altitude bias (not used as morphology)

    Channels are authored independently. B is never derived from R.
    R is a continuous soft field — no Worley island carve, no hard binary
    sky mask, no footprint/size/spacing generator.
    """
    coords = _make_coords(size)

    # --- R: Coverage scalar — continuous soft weather amount (no islands) ---
    LOG.info("  R coverage scalar (Perlin FBM freq=%d, %d octaves) ...", coverage_freq, coverage_octaves)
    coverage_soft = perlin_fbm2d01(
        coords,
        base_frequency=coverage_freq,
        octaves=coverage_octaves,
        lacunarity=2.0,
        gain=0.52,
        seed=seed,
    )
    coverage_detail = perlin_fbm2d01(
        coords,
        base_frequency=coverage_freq * 4,
        octaves=3,
        gain=0.45,
        seed=seed + 111,
    )
    # Soft continuous field — amount only, not a silhouette / island mask.
    # Contrast-stretch the same Perlin topology so clear-air lows and dense
    # banks coexist (HZD coverage amount), without Worley island carving.
    coverage = coverage_soft * 0.78 + coverage_detail * 0.22
    # Stronger amount contrast on the same Perlin topology (clear lows + dense banks).
    coverage = remap_clamped(coverage, 0.32, 0.68, 0.0, 1.0)
    coverage = np.clip(np.power(coverage, 1.08), 0.0, 1.0)

    # --- G: Precipitation — independent storm field (shader does precip→CB) ---
    LOG.info("  G precipitation (independent of type; soft clear-air damp only) ...")
    precip_macro = perlin_fbm2d01(
        coords,
        base_frequency=max(1, coverage_freq),
        octaves=4,
        seed=seed + 333,
    )
    precip_local = perlin_fbm2d01(
        coords,
        base_frequency=coverage_freq * 3,
        octaves=3,
        seed=seed + 334,
    )
    precip = precip_macro * 0.65 + precip_local * 0.35
    precip = remap_clamped(precip, 0.30, 0.75, 0.0, 1.0) * precip_strength
    # Mild physical damp in near-zero coverage air — not a morphology gate.
    clear_damp = np.clip((coverage - 0.04) / 0.18, 0.0, 1.0)
    precip = np.clip(precip * (0.25 + 0.75 * clear_damp), 0.0, 1.0)

    # --- B: Cloud type — INDEPENDENT of R (and of G in the texture) ---
    # Shader applies documented precip→CB from G. Do not bake precip into B.
    LOG.info("  B cloud type (independent Perlin continuum St..Cb) ...")
    type_macro = perlin_fbm2d01(
        coords,
        base_frequency=max(1, coverage_freq),
        octaves=4,
        seed=seed + 777,
    )
    type_local = perlin_fbm2d01(
        coords,
        base_frequency=coverage_freq * 3,
        octaves=3,
        seed=seed + 888,
    )
    cloud_type = type_macro * 0.50 + type_local * 0.50
    # Stretch to full documented continuum: 0 stratus … ~0.5 cumulus … 1 CB.
    cloud_type = remap_clamped(cloud_type, 0.32, 0.68, 0.0, 1.0)
    cloud_type = np.clip(cloud_type, 0.0, 1.0)

    # --- A: Neutral altitude bias (not a formation / size channel) ---
    LOG.info("  A altitude bias (neutral) ...")
    altitude = np.full_like(coverage, 1.0, dtype=np.float64)

    rgba = np.stack([coverage, precip, cloud_type, altitude], axis=-1).astype(np.float32)
    return rgba


def float_to_u8(arr: np.ndarray) -> np.ndarray:
    return np.clip(np.rint(arr * 255.0), 0, 255).astype(np.uint8)


# ===========================================================================
# Native DDS writer (DX10 extended header, R8G8B8A8_UNORM, TEXTURE2D)
# ===========================================================================

DXGI_FORMAT_R8G8B8A8_UNORM = 28
D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3

DDSD_CAPS = 0x1
DDSD_HEIGHT = 0x2
DDSD_WIDTH = 0x4
DDSD_PITCH = 0x8
DDSD_PIXELFORMAT = 0x1000
DDSD_MIPMAPCOUNT = 0x20000
DDSCAPS_TEXTURE = 0x1000
DDPF_FOURCC = 0x4


def _fourcc(tag: str) -> int:
    return struct.unpack("<I", tag.encode("ascii"))[0]


def write_dds_rgba8_2d(path: Path, pixels: np.ndarray, *, width: int, height: int) -> int:
    """Write DXGI_FORMAT_R8G8B8A8_UNORM 2D DDS with DDS_HEADER + DDS_HEADER_DXT10."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)

    expected = (height, width, 4)
    if pixels.shape != expected:
        raise ValueError(f"pixel shape {pixels.shape} != expected {expected}")
    if pixels.dtype != np.uint8:
        pixels = np.clip(pixels, 0, 255).astype(np.uint8)

    pitch = width * 4
    flags = (
        DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT | DDSD_PITCH
    )

    pf = struct.pack(
        "<IIIIIIII",
        32,
        DDPF_FOURCC,
        _fourcc("DX10"),
        0, 0, 0, 0, 0,
    )

    header = struct.pack(
        "<IIIIIII",
        124,
        flags,
        height,
        width,
        pitch,
        0,  # depth
        1,  # mipmap count
    )
    header += struct.pack("<11I", *([0] * 11))
    header += pf
    header += struct.pack("<IIIII", DDSCAPS_TEXTURE, 0, 0, 0, 0)
    assert len(header) == 124

    # DDS_HEADER_DXT10 (20 bytes)
    dx10 = struct.pack(
        "<IIIII",
        DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D10_RESOURCE_DIMENSION_TEXTURE2D,
        0,  # miscFlag
        1,  # arraySize
        0,  # miscFlags2
    )
    assert len(dx10) == 20

    raw = np.ascontiguousarray(pixels).tobytes()
    with path.open("wb") as f:
        f.write(b"DDS ")
        f.write(header)
        f.write(dx10)
        f.write(raw)

    return path.stat().st_size


# ===========================================================================
# Bake + CLI
# ===========================================================================

@dataclass(frozen=True)
class BakeResult:
    path: Path
    width: int
    height: int
    bytes_on_disk: int
    elapsed_s: float
    coverage_mean: float
    sky_fraction: float


def bake_weather_map(
    outdir: Path,
    *,
    size: int = DEFAULT_SIZE,
    seed: int = 2026,
    filename: str = DEFAULT_FILENAME,
) -> BakeResult:
    t0 = time.perf_counter()
    out_path = outdir / filename
    LOG.info("Baking %s (%d x %d) ...", out_path.name, size, size)

    rgba_f = generate_weather_map(size=size, seed=seed)
    rgba = np.stack(
        [
            float_to_u8(rgba_f[..., 0]),
            float_to_u8(rgba_f[..., 1]),
            float_to_u8(rgba_f[..., 2]),
            float_to_u8(rgba_f[..., 3]),
        ],
        axis=-1,
    )

    nbytes = write_dds_rgba8_2d(out_path, rgba, width=size, height=size)
    elapsed = time.perf_counter() - t0

    cov = rgba_f[..., 0]
    sky_fraction = float(np.mean(cov < 0.05))
    coverage_mean = float(np.mean(cov))

    LOG.info(
        "  wrote %s  (%d x %d, %.2f KiB, %.2fs)",
        out_path,
        size,
        size,
        nbytes / 1024.0,
        elapsed,
    )
    LOG.info(
        "  coverage mean=%.3f  sky openings=%.1f%%  precip mean=%.3f  type mean=%.3f",
        coverage_mean,
        sky_fraction * 100.0,
        float(np.mean(rgba_f[..., 1])),
        float(np.mean(rgba_f[..., 2])),
    )
    return BakeResult(out_path, size, size, nbytes, elapsed, coverage_mean, sky_fraction)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description=(
            "Bake a Horizon Zero Dawn / Nubis 2D weather map "
            "(CloudWeatherMap1024.dds) as DXGI_FORMAT_R8G8B8A8_UNORM DDS."
        ),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    p.add_argument(
        "--outdir",
        type=Path,
        required=True,
        help="Engine content directory (texture is written directly here)",
    )
    p.add_argument(
        "--size",
        type=int,
        default=DEFAULT_SIZE,
        help="Texture resolution (width = height)",
    )
    p.add_argument(
        "--seed",
        type=int,
        default=2026,
        help="Procedural RNG seed",
    )
    p.add_argument(
        "--filename",
        type=str,
        default=DEFAULT_FILENAME,
        help="Output DDS filename",
    )
    p.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Debug logging",
    )
    return p.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    _configure_logging(args.verbose)

    if args.size < 16 or args.size > 8192:
        LOG.error("--size must be in [16, 8192]")
        return 2

    outdir: Path = args.outdir
    outdir.mkdir(parents=True, exist_ok=True)
    LOG.info("Output directory: %s", outdir.resolve())

    result = bake_weather_map(
        outdir,
        size=args.size,
        seed=args.seed,
        filename=args.filename,
    )

    LOG.info(
        "Done in %.2fs — %s (sky openings %.1f%%)",
        result.elapsed_s,
        result.path.name,
        result.sky_fraction * 100.0,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
