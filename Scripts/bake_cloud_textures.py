#!/usr/bin/env python3
# ==============================================================================
# WindEffects — Horizon Zero Dawn / Nubis Cloud Noise Texture Baker
#
# Procedurally generates the three Schneider (SIGGRAPH 2015 / GPU Pro 7) noise
# assets used by Guerrilla's volumetric cloudscapes:
#   - CloudBaseShape128.dds      (128^3 RGBA8 volume)
#   - CloudDetailErosion32.dds   (32^3  RGBA8 volume)
#   - CloudTurbulenceCurl128.dds (128^2 RGBA8 2D)
#
# All noise is strictly periodic so WRAP sampling has zero edge seams.
# DDS containers use DX10 extended headers (DXGI_FORMAT_R8G8B8A8_UNORM).
#
# Usage:
#   python bake_cloud_textures.py --outdir <path/to/engine/content>
# ==============================================================================

from __future__ import annotations

import argparse
import logging
import struct
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Tuple

import numpy as np

# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------

LOG = logging.getLogger("cloud_baker")


def _configure_logging(verbose: bool) -> None:
    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(
        level=level,
        format="[%(asctime)s] %(levelname)s  %(message)s",
        datefmt="%H:%M:%S",
    )


# ===========================================================================
# 1. Schneider utility
# ===========================================================================

def remap(
    val: np.ndarray | float,
    orig_min: float,
    orig_max: float,
    new_min: np.ndarray | float,
    new_max: float,
) -> np.ndarray | float:
    """Exact Schneider Remap (GPU Pro 7 / Nubis).

    Remap(val, origMin, origMax, newMin, newMax) =
        newMin + ((val - origMin) / (origMax - origMin)) * (newMax - newMin)
    """
    return new_min + ((val - orig_min) / (orig_max - orig_min)) * (new_max - new_min)


# ===========================================================================
# 2. Hash / gradient helpers (periodic)
# ===========================================================================

def _hash3(ix: np.ndarray, iy: np.ndarray, iz: np.ndarray, seed: int = 1337) -> np.ndarray:
    """Deterministic 3D integer hash -> float in [0, 1)."""
    n = (
        ix.astype(np.int64) * 73856093
        ^ iy.astype(np.int64) * 19349663
        ^ iz.astype(np.int64) * 83492791
        ^ int(seed)
    )
    # Knuth multiplicative hash, keep unsigned wrap semantics
    n = (n * 2654435761) & 0xFFFFFFFF
    return n.astype(np.float64) / 4294967296.0


def _hash2(ix: np.ndarray, iy: np.ndarray, seed: int = 1337) -> np.ndarray:
    n = (
        ix.astype(np.int64) * 73856093
        ^ iy.astype(np.int64) * 19349663
        ^ int(seed)
    )
    n = (n * 2654435761) & 0xFFFFFFFF
    return n.astype(np.float64) / 4294967296.0


def _grad3_table() -> np.ndarray:
    """Unit-ish 3D gradient set (Perlin-style). Shape (12, 3)."""
    g = np.array(
        [
            [1, 1, 0], [-1, 1, 0], [1, -1, 0], [-1, -1, 0],
            [1, 0, 1], [-1, 0, 1], [1, 0, -1], [-1, 0, -1],
            [0, 1, 1], [0, -1, 1], [0, 1, -1], [0, -1, -1],
        ],
        dtype=np.float64,
    )
    return g


_GRAD3 = _grad3_table()


def _fade(t: np.ndarray) -> np.ndarray:
    """Perlin quintic fade: 6t^5 - 15t^4 + 10t^3."""
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0)


# ===========================================================================
# 3. Seamless 3D / 2D Perlin
# ===========================================================================

def perlin3d_periodic(
    coords: np.ndarray,
    frequency: int,
    seed: int = 1337,
) -> np.ndarray:
    """Periodic 3D classic Perlin noise.

    Parameters
    ----------
    coords : (..., 3) float array in [0, 1) texture space.
    frequency : integer cell count across the unit cube (period).
    Returns values roughly in [-1, 1].
    """
    if frequency < 1:
        raise ValueError("frequency must be >= 1")

    p = coords * float(frequency)
    cell = np.floor(p).astype(np.int64)
    f = p - cell.astype(np.float64)
    u = _fade(f)

    # 8 lattice corners, wrapped into [0, frequency)
    dots = []
    for dz in (0, 1):
        for dy in (0, 1):
            for dx in (0, 1):
                ix = (cell[..., 0] + dx) % frequency
                iy = (cell[..., 1] + dy) % frequency
                iz = (cell[..., 2] + dz) % frequency
                h = _hash3(ix, iy, iz, seed=seed)
                gi = (h * 12.0).astype(np.int64) % 12
                g = _GRAD3[gi]  # (..., 3)
                offset = f - np.array([dx, dy, dz], dtype=np.float64)
                dots.append(np.sum(g * offset, axis=-1))

    # Trilinear blend of the 8 corner dots
    # Order: (0,0,0),(1,0,0),(0,1,0),(1,1,0),(0,0,1),(1,0,1),(0,1,1),(1,1,1)
    n000, n100, n010, n110, n001, n101, n011, n111 = dots
    nx00 = n000 + u[..., 0] * (n100 - n000)
    nx10 = n010 + u[..., 0] * (n110 - n010)
    nx01 = n001 + u[..., 0] * (n101 - n001)
    nx11 = n011 + u[..., 0] * (n111 - n011)
    nxy0 = nx00 + u[..., 1] * (nx10 - nx00)
    nxy1 = nx01 + u[..., 1] * (nx11 - nx01)
    return nxy0 + u[..., 2] * (nxy1 - nxy0)


def perlin2d_periodic(
    coords: np.ndarray,
    frequency: int,
    seed: int = 1337,
) -> np.ndarray:
    """Periodic 2D classic Perlin. coords (..., 2) in [0,1). Returns ~[-1,1]."""
    if frequency < 1:
        raise ValueError("frequency must be >= 1")

    grads = np.array(
        [[1, 0], [-1, 0], [0, 1], [0, -1],
         [0.7071, 0.7071], [-0.7071, 0.7071],
         [0.7071, -0.7071], [-0.7071, -0.7071]],
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


def perlin_fbm3d(
    coords: np.ndarray,
    base_frequency: int = 4,
    octaves: int = 4,
    lacunarity: float = 2.0,
    gain: float = 0.5,
    seed: int = 1337,
) -> np.ndarray:
    """Fractal Brownian Motion of periodic 3D Perlin, remapped to [0, 1]."""
    amp = 1.0
    amp_sum = 0.0
    total = np.zeros(coords.shape[:-1], dtype=np.float64)
    freq = float(base_frequency)

    for o in range(octaves):
        f_int = max(1, int(round(freq)))
        # Keep period exact: frequency must tile the unit cube
        if abs(freq - f_int) > 1e-6:
            f_int = max(1, int(freq))
        n = perlin3d_periodic(coords, frequency=f_int, seed=seed + o * 101)
        total += n * amp
        amp_sum += amp
        amp *= gain
        freq *= lacunarity

    # Normalize to ~[-1,1] then map to [0,1]
    total = total / max(amp_sum, 1e-8)
    return np.clip(total * 0.5 + 0.5, 0.0, 1.0)


def perlin_fbm2d(
    coords: np.ndarray,
    base_frequency: int = 4,
    octaves: int = 4,
    lacunarity: float = 2.0,
    gain: float = 0.5,
    seed: int = 42,
) -> np.ndarray:
    """2D Perlin FBM in ~[-1, 1] (not remapped) — used as curl potential."""
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

    return total / max(amp_sum, 1e-8)


# ===========================================================================
# 4. Seamless 3D Worley (cellular) noise
# ===========================================================================

def worley3d_periodic(
    coords: np.ndarray,
    cell_frequency: int,
    seed: int = 9001,
    invert: bool = False,
) -> np.ndarray:
    """Periodic 3D Worley (F1 Euclidean distance).

    Parameters
    ----------
    coords : (..., 3) in [0, 1).
    cell_frequency : integer number of cells across the unit cube.
    invert : if True, return 1 - normalized distance (billowy).

    Returns values in [0, 1].
    """
    if cell_frequency < 1:
        raise ValueError("cell_frequency must be >= 1")

    p = coords * float(cell_frequency)
    cell = np.floor(p).astype(np.int64)

    min_d2 = np.full(coords.shape[:-1], np.inf, dtype=np.float64)

    for oz in (-1, 0, 1):
        for oy in (-1, 0, 1):
            for ox in (-1, 0, 1):
                ncell = cell + np.array([ox, oy, oz], dtype=np.int64)
                # Wrap cell id for hashing (periodic lattice)
                wrapped = np.mod(ncell, cell_frequency)
                # Feature point jitter in [0, 1)^3 inside each cell
                jx = _hash3(wrapped[..., 0], wrapped[..., 1], wrapped[..., 2], seed=seed)
                jy = _hash3(wrapped[..., 0], wrapped[..., 1], wrapped[..., 2], seed=seed + 17)
                jz = _hash3(wrapped[..., 0], wrapped[..., 1], wrapped[..., 2], seed=seed + 31)
                feature = ncell.astype(np.float64) + np.stack([jx, jy, jz], axis=-1)
                delta = feature - p
                d2 = np.sum(delta * delta, axis=-1)
                min_d2 = np.minimum(min_d2, d2)

    # Max possible F1 distance inside a cell neighbourhood is bounded; normalize
    # by sqrt(3) so results sit comfortably in [0, 1] before optional invert.
    dist = np.sqrt(min_d2) / np.sqrt(3.0)
    dist = np.clip(dist, 0.0, 1.0)
    if invert:
        return 1.0 - dist
    return dist


def worley_fbm3d(
    coords: np.ndarray,
    base_frequency: int = 4,
    octaves: int = 3,
    invert: bool = False,
    seed: int = 9001,
) -> np.ndarray:
    """Worley FBM with frequency doubling / amplitude weights 0.625, 0.25, 0.125."""
    weights = [0.625, 0.25, 0.125]
    if octaves > len(weights):
        # Extend with geometric decay if more octaves requested
        w = 0.125
        for _ in range(octaves - len(weights)):
            w *= 0.5
            weights.append(w)

    total = np.zeros(coords.shape[:-1], dtype=np.float64)
    freq = base_frequency
    wsum = 0.0
    for o in range(octaves):
        w = weights[o]
        total += w * worley3d_periodic(
            coords, cell_frequency=freq, seed=seed + o * 13, invert=invert
        )
        wsum += w
        freq *= 2

    return total / max(wsum, 1e-8)


# ===========================================================================
# 5. Perlin–Worley (HZD / Schneider SIGGRAPH 2015 — Remap construction)
# ===========================================================================

def worley_fbm_from_layers(
    w_lo: np.ndarray,
    w_mid: np.ndarray,
    w_hi: np.ndarray,
) -> np.ndarray:
    """Inverted-Worley FBM weights matching runtime StageWorleyFbm (0.625/0.25/0.125)."""
    return np.clip(w_lo * 0.625 + w_mid * 0.250 + w_hi * 0.125, 0.0, 1.0)


# Back-compat alias for older audit scripts.
worley_billow_fbm_from_layers = worley_fbm_from_layers


def perlin_worley_remap(
    perlin: np.ndarray,
    worley_fbm: np.ndarray,
) -> np.ndarray:
    """HZD Texture1.R = Remap(perlin, 1 - worleyFBM, 1, 0, 1).

    This is the documented Schneider Perlin-Worley combination — NOT additive
    dilation, NOT a custom morphology generator, NOT world-specific formation.
    """
    p = np.clip(perlin, 0.0, 1.0)
    w = np.clip(worley_fbm, 0.0, 1.0)
    floor = np.clip(1.0 - w, 0.0, 1.0)
    return np.clip(remap(p, floor, 1.0, 0.0, 1.0), 0.0, 1.0)


def perlin_worley(
    coords: np.ndarray,
    perlin_base_freq: int = 2,
    perlin_octaves: int = 4,
    worley_base_freq: int = 8,
    worley_octaves: int = 3,
    seed: int = 1337,
    **_ignored,
) -> np.ndarray:
    """Bake-only helper: classic HZD Remap(perlinFBM, 1 - worleyFBM, 1)."""
    p = perlin_fbm3d(
        coords,
        base_frequency=perlin_base_freq,
        octaves=perlin_octaves,
        seed=seed,
    )
    f0 = max(1, int(worley_base_freq))
    w_lo = worley3d_periodic(coords, cell_frequency=f0, invert=True, seed=seed + 500)
    w_mid = worley3d_periodic(coords, cell_frequency=f0 * 2, invert=True, seed=seed + 513)
    w_hi = worley3d_periodic(coords, cell_frequency=f0 * 4, invert=True, seed=seed + 526)
    if worley_octaves <= 1:
        w_fbm = w_lo
    elif worley_octaves == 2:
        w_fbm = np.clip(w_lo * 0.75 + w_mid * 0.25, 0.0, 1.0)
    else:
        w_fbm = worley_fbm_from_layers(w_lo, w_mid, w_hi)
    return perlin_worley_remap(p, w_fbm)


# ===========================================================================
# 6. 2D Curl noise (divergence-free)
# ===========================================================================

def curl_noise_2d(
    resolution: int = 128,
    potential_freq: int = 4,
    potential_octaves: int = 4,
    seed: int = 42,
) -> Tuple[np.ndarray, np.ndarray]:
    """Build a periodic 2D curl vector field from a scalar potential.

    Curl(P) = (dP/dy, -dP/dx) via central finite differences with WRAP.
    Returns (vx, vy) each shaped (H, W), already mapped into [0, 1] UNORM.
    """
    # Evaluate potential on a slightly denser grid is unnecessary; sample at
    # texel centers in [0, 1) and difference with modular neighbours.
    xs = (np.arange(resolution, dtype=np.float64) + 0.5) / resolution
    ys = (np.arange(resolution, dtype=np.float64) + 0.5) / resolution
    xx, yy = np.meshgrid(xs, ys, indexing="xy")
    coords = np.stack([xx, yy], axis=-1)

    potential = perlin_fbm2d(
        coords,
        base_frequency=potential_freq,
        octaves=potential_octaves,
        seed=seed,
    )

    # Finite differences with wrap (periodic)
    # dP/dx ≈ (P[x+1] - P[x-1]) / (2 * dx), dx = 1/resolution
    dx = 1.0 / resolution
    dpd_x = (np.roll(potential, -1, axis=1) - np.roll(potential, 1, axis=1)) / (2.0 * dx)
    dpd_y = (np.roll(potential, -1, axis=0) - np.roll(potential, 1, axis=0)) / (2.0 * dx)

    # 2D curl of a scalar potential treated as z-component of a 3D field:
    # curl = (dP/dy, -dP/dx)  → divergence-free in 2D
    vx = dpd_y
    vy = -dpd_x

    # Map from signed range into UNORM [0, 1]. Scale by a robust percentile
    # so extreme outliers don't crush the field, then bias to 0.5.
    mag = np.maximum(np.abs(vx).max(), np.abs(vy).max())
    if mag < 1e-12:
        mag = 1.0
    vx_n = np.clip(vx / mag, -1.0, 1.0)
    vy_n = np.clip(vy / mag, -1.0, 1.0)
    return 0.5 + 0.5 * vx_n, 0.5 + 0.5 * vy_n


# ===========================================================================
# 7. Native DDS writer (DX10 extended header)
# ===========================================================================

# DXGI / D3D10 constants
DXGI_FORMAT_R8G8B8A8_UNORM = 28
D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3
D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4

DDSD_CAPS = 0x1
DDSD_HEIGHT = 0x2
DDSD_WIDTH = 0x4
DDSD_PITCH = 0x8
DDSD_PIXELFORMAT = 0x1000
DDSD_MIPMAPCOUNT = 0x20000
DDSD_DEPTH = 0x800000

DDSCAPS_TEXTURE = 0x1000
DDSCAPS2_VOLUME = 0x200000

DDPF_FOURCC = 0x4


def _fourcc(tag: str) -> int:
    return struct.unpack("<I", tag.encode("ascii"))[0]


def write_dds_rgba8(
    path: Path,
    pixels: np.ndarray,
    *,
    width: int,
    height: int,
    depth: int = 1,
) -> int:
    """Write an RGBA8 UNORM DDS with DX10 header.

    pixels : uint8 array
        - 2D: (H, W, 4)
        - 3D: (D, H, W, 4)  — depth slices outermost (z, y, x, c)
    Returns number of bytes written to disk.
    """
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)

    is_volume = depth > 1
    if is_volume:
        expected = (depth, height, width, 4)
    else:
        expected = (height, width, 4)

    if pixels.shape != expected:
        raise ValueError(f"pixel shape {pixels.shape} != expected {expected}")

    if pixels.dtype != np.uint8:
        pixels = np.clip(pixels, 0, 255).astype(np.uint8)

    pitch = width * 4
    mipmap_count = 1
    flags = (
        DDSD_CAPS
        | DDSD_HEIGHT
        | DDSD_WIDTH
        | DDSD_PIXELFORMAT
        | DDSD_MIPMAPCOUNT
        | DDSD_PITCH
    )
    caps = DDSCAPS_TEXTURE
    caps2 = 0
    if is_volume:
        flags |= DDSD_DEPTH
        caps2 |= DDSCAPS2_VOLUME

    # DDS_PIXELFORMAT (32 bytes) — FourCC = 'DX10'
    pf = struct.pack(
        "<IIIIIIII",
        32,                 # dwSize
        DDPF_FOURCC,        # dwFlags
        _fourcc("DX10"),    # dwFourCC
        0, 0, 0, 0, 0,      # RGB bitmasks unused
    )

    # DDS_HEADER (124 bytes) — dwSize + 30 uint32 + pf already counted
    # Layout: dwSize, dwFlags, dwHeight, dwWidth, dwPitchOrLinearSize,
    #         dwDepth, dwMipMapCount, dwReserved1[11], ddspf(32),
    #         dwCaps, dwCaps2, dwCaps3, dwCaps4, dwReserved2
    header = struct.pack(
        "<IIIIIII",
        124,            # dwSize
        flags,
        height,
        width,
        pitch,          # dwPitchOrLinearSize
        depth if is_volume else 0,
        mipmap_count,
    )
    header += struct.pack("<11I", *([0] * 11))  # dwReserved1
    header += pf
    header += struct.pack(
        "<IIIII",
        caps,
        caps2,
        0,  # dwCaps3
        0,  # dwCaps4
        0,  # dwReserved2
    )
    assert len(header) == 124

    dimension = (
        D3D10_RESOURCE_DIMENSION_TEXTURE3D
        if is_volume
        else D3D10_RESOURCE_DIMENSION_TEXTURE2D
    )
    dx10 = struct.pack(
        "<IIIII",
        DXGI_FORMAT_R8G8B8A8_UNORM,  # dxgiFormat
        dimension,                    # resourceDimension
        0,                            # miscFlag
        1,                            # arraySize
        0,                            # miscFlags2
    )
    assert len(dx10) == 20

    raw = np.ascontiguousarray(pixels).tobytes()
    with path.open("wb") as f:
        f.write(b"DDS ")
        f.write(header)
        f.write(dx10)
        f.write(raw)

    return path.stat().st_size


def float_to_u8(arr: np.ndarray) -> np.ndarray:
    return np.clip(np.rint(arr * 255.0), 0, 255).astype(np.uint8)


# ===========================================================================
# 8. Volume / image bakers
# ===========================================================================

@dataclass(frozen=True)
class BakeResult:
    path: Path
    width: int
    height: int
    depth: int
    bytes_on_disk: int
    elapsed_s: float


def _make_volume_coords(size: int) -> np.ndarray:
    """Texel-center coordinates in [0, 1)^3, shape (D, H, W, 3)."""
    t = (np.arange(size, dtype=np.float64) + 0.5) / size
    # Indexing: z, y, x
    zz, yy, xx = np.meshgrid(t, t, t, indexing="ij")
    return np.stack([xx, yy, zz], axis=-1)


def bake_cloud_base_shape(outdir: Path, size: int = 128) -> BakeResult:
    """CloudBaseShape128 — HZD Texture1 packing (reusable, not world-specific).

      R     = Perlin-Worley = Remap(perlinFBM, 1 - worleyFBM, 1)
      G/B/A = inverted Worley @ increasing frequency (8 / 16 / 32)

    Frequency hierarchy keeps Perlin as coarse mass and Worley as finer cells.
    Appearance changes at runtime via weather/type/height — not by rebaking.
    """
    t0 = time.perf_counter()
    out_path = outdir / "CloudBaseShape128.dds"
    LOG.info("Baking %s (%d^3) — HZD Perlin-Worley Remap ...", out_path.name, size)

    coords = _make_volume_coords(size)

    perlin_base_freq = 2
    worley_g_freq = 8

    LOG.info("  Perlin FBM (base_freq=%d) ...", perlin_base_freq)
    perlin = perlin_fbm3d(
        coords, base_frequency=perlin_base_freq, octaves=4, seed=1337
    )

    LOG.info("  Worley G (cell freq=%d) ...", worley_g_freq)
    g = worley3d_periodic(
        coords, cell_frequency=worley_g_freq, invert=True, seed=9100
    )
    LOG.info("  Worley B (cell freq=%d) ...", worley_g_freq * 2)
    b = worley3d_periodic(
        coords, cell_frequency=worley_g_freq * 2, invert=True, seed=9200
    )
    LOG.info("  Worley A (cell freq=%d) ...", worley_g_freq * 4)
    a = worley3d_periodic(
        coords, cell_frequency=worley_g_freq * 4, invert=True, seed=9300
    )

    w_fbm = worley_fbm_from_layers(g, b, a)
    LOG.info("  R = Remap(perlin, 1 - worleyFBM, 1) ...")
    r = perlin_worley_remap(perlin, w_fbm)

    rgba = np.stack(
        [float_to_u8(r), float_to_u8(g), float_to_u8(b), float_to_u8(a)],
        axis=-1,
    )
    nbytes = write_dds_rgba8(out_path, rgba, width=size, height=size, depth=size)

    # HZD Texture1 bake-stage debug (not custom morphology)
    dbg = Path("Build/Audit/StageAudit/HZD_Texture1_Bake")
    try:
        from PIL import Image

        dbg.mkdir(parents=True, exist_ok=True)
        period_m = 3200.0
        extent_m = 9600.0
        res = 256
        xs = np.linspace(0.0, extent_m, res, endpoint=False)
        zs = np.linspace(0.0, extent_m, res, endpoint=False)
        xx, zz = np.meshgrid(xs, zs, indexing="xy")
        yy = np.full_like(xx, 0.5 * period_m)
        D, H, W = r.shape
        ui = ((xx / period_m) * W).astype(np.int64) % W
        vi = ((yy / period_m) * H).astype(np.int64) % H
        wi = ((zz / period_m) * D).astype(np.int64) % D

        def _world(vol: np.ndarray) -> np.ndarray:
            return vol[wi, vi, ui]

        world_stages = {
            "01_raw_Perlin": _world(perlin),
            "02_Worley_FBM": _world(w_fbm),
            "03_Remap_floor_1_minus_Worley": _world(np.clip(1.0 - w_fbm, 0.0, 1.0)),
            "04_Texture1_R_PerlinWorley": _world(r),
        }
        for name, sl in world_stages.items():
            u8 = np.clip(np.rint(sl * 255.0), 0, 255).astype(np.uint8)
            Image.fromarray(u8, mode="L").save(dbg / f"{name}_world9600m.png")

        mid = size // 2
        for name, vol in (
            ("01_raw_Perlin", perlin),
            ("02_Worley_FBM", w_fbm),
            ("03_Remap_floor_1_minus_Worley", np.clip(1.0 - w_fbm, 0.0, 1.0)),
            ("04_Texture1_R_PerlinWorley", r),
        ):
            for plane, sl in (
                ("XY", vol[mid]),
                ("XZ", vol[:, mid, :]),
                ("YZ", vol[:, :, mid]),
            ):
                u8 = np.clip(np.rint(sl * 255.0), 0, 255).astype(np.uint8)
                Image.fromarray(u8, mode="L").save(dbg / f"{name}_{plane}.png")
        LOG.info("  wrote HZD Texture1 bake debug -> %s", dbg)
    except Exception as exc:  # noqa: BLE001 — debug export must not fail the bake
        LOG.warning("  Texture1 bake debug export skipped: %s", exc)

    elapsed = time.perf_counter() - t0
    LOG.info(
        "  wrote %s  (%d x %d x %d, %.2f MiB, %.1fs)",
        out_path,
        size,
        size,
        size,
        nbytes / (1024 * 1024),
        elapsed,
    )
    return BakeResult(out_path, size, size, size, nbytes, elapsed)


def bake_cloud_detail_erosion(outdir: Path, size: int = 32) -> BakeResult:
    """CloudDetailErosion32 — R/G/B Worley @ 2/4/8, A=1."""
    t0 = time.perf_counter()
    out_path = outdir / "CloudDetailErosion32.dds"
    LOG.info("Baking %s (%d^3) ...", out_path.name, size)

    coords = _make_volume_coords(size)
    LOG.info("  computing Worley R (cell freq=2) ...")
    r = worley3d_periodic(coords, cell_frequency=2, invert=True, seed=8100)
    LOG.info("  computing Worley G (cell freq=4) ...")
    g = worley3d_periodic(coords, cell_frequency=4, invert=True, seed=8200)
    LOG.info("  computing Worley B (cell freq=8) ...")
    b = worley3d_periodic(coords, cell_frequency=8, invert=True, seed=8300)
    a = np.ones(coords.shape[:-1], dtype=np.float64)

    rgba = np.stack(
        [float_to_u8(r), float_to_u8(g), float_to_u8(b), float_to_u8(a)],
        axis=-1,
    )
    nbytes = write_dds_rgba8(out_path, rgba, width=size, height=size, depth=size)
    elapsed = time.perf_counter() - t0
    LOG.info(
        "  wrote %s  (%d x %d x %d, %.2f KiB, %.1fs)",
        out_path,
        size,
        size,
        size,
        nbytes / 1024,
        elapsed,
    )
    return BakeResult(out_path, size, size, size, nbytes, elapsed)


def bake_cloud_turbulence_curl(outdir: Path, size: int = 128) -> BakeResult:
    """CloudTurbulenceCurl128 — RG=curl vector UNORM, B=0.5, A=1."""
    t0 = time.perf_counter()
    out_path = outdir / "CloudTurbulenceCurl128.dds"
    LOG.info("Baking %s (%d^2) ...", out_path.name, size)

    LOG.info("  computing 2D curl noise ...")
    vx, vy = curl_noise_2d(resolution=size, potential_freq=4, potential_octaves=4)
    b = np.full((size, size), 0.5, dtype=np.float64)
    a = np.ones((size, size), dtype=np.float64)

    rgba = np.stack(
        [float_to_u8(vx), float_to_u8(vy), float_to_u8(b), float_to_u8(a)],
        axis=-1,
    )
    nbytes = write_dds_rgba8(out_path, rgba, width=size, height=size, depth=1)
    elapsed = time.perf_counter() - t0
    LOG.info(
        "  wrote %s  (%d x %d, %.2f KiB, %.1fs)",
        out_path,
        size,
        size,
        nbytes / 1024,
        elapsed,
    )
    return BakeResult(out_path, size, size, 1, nbytes, elapsed)


# ===========================================================================
# 9. CLI
# ===========================================================================

def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description=(
            "Bake Horizon Zero Dawn / Nubis cloud noise textures "
            "(CloudBaseShape128, CloudDetailErosion32, CloudTurbulenceCurl128) "
            "as DX10 RGBA8 DDS files."
        ),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    p.add_argument(
        "--outdir",
        type=Path,
        required=True,
        help="Engine content directory (textures are written directly here)",
    )
    p.add_argument(
        "--skip-base",
        action="store_true",
        help="Skip CloudBaseShape128 bake",
    )
    p.add_argument(
        "--skip-detail",
        action="store_true",
        help="Skip CloudDetailErosion32 bake",
    )
    p.add_argument(
        "--skip-curl",
        action="store_true",
        help="Skip CloudTurbulenceCurl128 bake",
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

    outdir: Path = args.outdir
    outdir.mkdir(parents=True, exist_ok=True)
    LOG.info("Output directory: %s", outdir.resolve())

    results: list[BakeResult] = []
    t_all = time.perf_counter()

    if not args.skip_base:
        results.append(bake_cloud_base_shape(outdir))
    if not args.skip_detail:
        results.append(bake_cloud_detail_erosion(outdir))
    if not args.skip_curl:
        results.append(bake_cloud_turbulence_curl(outdir))

    if not results:
        LOG.warning("Nothing to bake (all assets skipped).")
        return 0

    total_bytes = sum(r.bytes_on_disk for r in results)
    LOG.info("-" * 60)
    LOG.info("Bake complete in %.1fs", time.perf_counter() - t_all)
    for r in results:
        dim = (
            f"{r.width}x{r.height}x{r.depth}"
            if r.depth > 1
            else f"{r.width}x{r.height}"
        )
        LOG.info(
            "  %-32s  %s  %s",
            r.path.name,
            dim,
            _fmt_bytes(r.bytes_on_disk),
        )
    LOG.info("  %-32s  %s", "TOTAL", _fmt_bytes(total_bytes))
    return 0


def _fmt_bytes(n: int) -> str:
    if n >= 1024 * 1024:
        return f"{n / (1024 * 1024):.2f} MiB"
    if n >= 1024:
        return f"{n / 1024:.2f} KiB"
    return f"{n} B"


if __name__ == "__main__":
    sys.exit(main())
