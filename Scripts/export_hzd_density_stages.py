#!/usr/bin/env python3
"""Offline HZD 2015 density-stage debug slices (mirrors SampleCloudDensity order).

Writes grayscale PNGs under Build/Audit/StageAudit/HZD_Density_Stages/ for:
  01 Texture1 R
  02 Perlin-Worley base (Remap R by Worley FBM)
  03 Height profile
  04 Coverage amount (WeatherMap.R)
  05 Base after height × coverage × bottom reduce
  06 Texture2 detail FBM
  07 Erosion delta
  08 Bottom wisps
  09 Curl magnitude
  10 Final density
  11 Precipitation (WeatherMap.G)
  12 Weather type (WeatherMap.B)

Also verifies WeatherMap alone cannot produce circular cloud footprints.
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Build/Audit/StageAudit/HZD_Density_Stages"
REPORT = OUT / "HZD_DENSITY_PIPELINE_REPORT.txt"

SHAPE = ROOT / "Engine/EngineContent/Cloud/CloudBaseShape128.dds"
DETAIL = ROOT / "Engine/EngineContent/Cloud/CloudDetailErosion32.dds"
CURL = ROOT / "Engine/EngineContent/Cloud/CloudTurbulenceCurl128.dds"
WEATHER = ROOT / "Engine/EngineContent/Cloud/CloudWeatherMap1024.dds"

# DaylightConfig defaults (structural — not visual tuning)
BASE_SCALE = 1.0 / 3200.0
DETAIL_SCALE = 0.0028
CURL_STRENGTH = 0.32
WEATHER_SCALE = 0.000025
EROSION = 0.20
WISP = 0.30
BOTTOM_REDUCE = 0.42
BASE_WORLEY = 1.0
COV_LO, COV_HI = 0.22, 0.78
TYPE_SHARP = 1.25
STRATO_SPAN = 3200.0
RES = 256
EXTENT_M = 9600.0
Y_AGL = 900.0  # mid strato sample altitude


def load_dds(path: Path) -> np.ndarray:
    data = path.read_bytes()
    header = data[4:128]
    height, width = struct.unpack_from("<II", header, 8)
    depth = struct.unpack_from("<I", header, 20)[0] or 1
    pf_flags = struct.unpack_from("<I", header, 80)[0]
    # DX10 extended header when FourCC is DX10
    fourcc = header[84:88]
    offset = 148 if fourcc == b"DX10" else 128
    raw = np.frombuffer(data[offset:], dtype=np.uint8)
    n = width * height * max(depth, 1) * 4
    arr = raw[:n].reshape(max(depth, 1), height, width, 4).astype(np.float64) / 255.0
    return arr


def sample3d(vol: np.ndarray, uvw: np.ndarray) -> np.ndarray:
    """Nearest-wrap sample. vol (D,H,W,C), uvw (...,3)."""
    d, h, w, c = vol.shape
    u = np.mod(uvw[..., 0], 1.0)
    v = np.mod(uvw[..., 1], 1.0)
    ww = np.mod(uvw[..., 2], 1.0)
    xi = (u * w).astype(np.int64) % w
    yi = (v * h).astype(np.int64) % h
    zi = (ww * d).astype(np.int64) % d
    return vol[zi, yi, xi]


def sample2d(img: np.ndarray, uv: np.ndarray) -> np.ndarray:
    """img (1,H,W,C) or (H,W,C)."""
    if img.ndim == 4:
        img = img[0]
    h, w, c = img.shape
    u = np.mod(uv[..., 0], 1.0)
    v = np.mod(uv[..., 1], 1.0)
    xi = (u * w).astype(np.int64) % w
    yi = (v * h).astype(np.int64) % h
    return img[yi, xi]


def remap_clamped(val, lo, hi, nlo=0.0, nhi=1.0):
    denom = np.maximum(np.asarray(hi - lo, dtype=np.float64), 1e-5)
    return np.clip(nlo + (val - lo) / denom * (nhi - nlo), nlo, nhi)


def stratus(h):
    return np.clip(
        np.clip((h - 0.00) / 0.10, 0, 1) * (1.0 - np.clip((h - 0.42) / 0.36, 0, 1)),
        0,
        1,
    )


def stratocumulus(h):
    deck = np.clip((h - 0.00) / 0.12, 0, 1) * (1.0 - np.clip((h - 0.55) / 0.33, 0, 1))
    rip = np.clip((h - 0.10) / 0.24, 0, 1) * (1.0 - np.clip((h - 0.52) / 0.28, 0, 1))
    return np.clip(deck * 0.88 + rip * 0.50, 0, 1)


def cumulus(h):
    body = np.clip((h - 0.00) / 0.12, 0, 1) * (1.0 - np.clip((h - 0.58) / 0.37, 0, 1))
    lobe = np.clip((h - 0.14) / 0.26, 0, 1) * (1.0 - np.clip((h - 0.68) / 0.30, 0, 1))
    top = np.clip((h - 0.28) / 0.24, 0, 1) * (1.0 - np.clip((h - 0.85) / 0.15, 0, 1))
    return np.clip(body * 0.92 + lobe * 0.75 + top * 0.50, 0, 1)


def cumulonimbus(h):
    base = np.clip((h - 0.00) / 0.10, 0, 1) * (1.0 - np.clip((h - 0.28) / 0.22, 0, 1))
    tower = np.clip((h - 0.06) / 0.24, 0, 1) * (1.0 - np.clip((h - 0.58) / 0.32, 0, 1))
    anvil = np.clip((h - 0.45) / 0.23, 0, 1) * (1.0 - np.clip((h - 0.88) / 0.12, 0, 1))
    return np.clip(base * 0.90 + tower * 0.95 + anvil * 0.65, 0, 1)


def type_weights(sig, sharp=1.25):
    t = np.clip(sig, 0, 1) * 3.0
    w_st = np.clip(1.0 - t, 0, 1)
    w_sc = np.clip(1.0 - np.abs(t - 1.0), 0, 1)
    w_cu = np.clip(1.0 - np.abs(t - 2.0), 0, 1)
    w_cb = np.clip(t - 2.0, 0, 1)
    s = max(float(sharp), 1.0)
    w_st = np.power(w_st, s)
    w_sc = np.power(w_sc, s)
    w_cu = np.power(w_cu, s)
    w_cb = np.power(w_cb, s)
    tot = np.maximum(w_st + w_sc + w_cu + w_cb, 1e-3)
    return w_st / tot, w_sc / tot, w_cu / tot, w_cb / tot


def save_gray(path: Path, arr: np.ndarray) -> None:
    u8 = np.clip(np.rint(np.clip(arr, 0, 1) * 255), 0, 255).astype(np.uint8)
    Image.fromarray(u8, mode="L").save(path)


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    missing = [p for p in (SHAPE, DETAIL, CURL, WEATHER) if not p.is_file()]
    if missing:
        print("FAIL: missing assets:")
        for p in missing:
            print(f"  {p}")
        return 1

    shape = load_dds(SHAPE)
    detail = load_dds(DETAIL)
    curl = load_dds(CURL)
    weather = load_dds(WEATHER)

    xs = np.linspace(0.0, EXTENT_M, RES, endpoint=False)
    zs = np.linspace(0.0, EXTENT_M, RES, endpoint=False)
    xx, zz = np.meshgrid(xs, zs, indexing="xy")
    yy = np.full_like(xx, Y_AGL)
    world = np.stack([xx, yy, zz], axis=-1)

    # Weather
    wuv = world[..., [0, 2]] * WEATHER_SCALE
    wsamp = sample2d(weather, wuv)
    cov = remap_clamped(wsamp[..., 0], COV_LO, COV_HI)
    precip = np.clip(wsamp[..., 1], 0, 1)
    type_sig = np.clip(wsamp[..., 2], 0, 1)
    w_st, w_sc, w_cu, w_cb = type_weights(type_sig, TYPE_SHARP)

    h_strato = np.clip(Y_AGL / STRATO_SPAN, 0, 1)
    h_cb = np.clip(Y_AGL / 9500.0, 0, 1)
    height = (
        w_st * stratus(h_strato)
        + w_sc * stratocumulus(h_strato)
        + w_cu * cumulus(h_strato)
        + w_cb * cumulonimbus(h_cb)
    )

    # Texture1
    suvw = world * BASE_SCALE
    s = sample3d(shape, suvw)
    tex1_r = s[..., 0]
    worley_fbm = np.clip(s[..., 1] * 0.625 + s[..., 2] * 0.25 + s[..., 3] * 0.125, 0, 1)
    carved = remap_clamped(tex1_r, worley_fbm - 1.0, 1.0)
    pw = np.clip(tex1_r * (1.0 - BASE_WORLEY) + carved * BASE_WORLEY, 0, 1)

    base = np.clip(pw * height, 0, 1)
    base = remap_clamped(base, np.clip(1.0 - cov, 0, 1), 1.0)
    bottom_fade = np.clip(h_strato / 0.22, 0, 1)
    bottom_mul = (1.0 - BOTTOM_REDUCE) + BOTTOM_REDUCE * bottom_fade
    # lerp(1-reduce, 1, bottomFade) = (1-reduce)*(1-f) + 1*f
    bottom_mul = (1.0 - BOTTOM_REDUCE) * (1.0 - bottom_fade) + 1.0 * bottom_fade
    base_after = np.clip(base * bottom_mul, 0, 1)

    # Curl + Texture2
    cuv = world[..., [0, 2]] * max(WEATHER_SCALE * 8.0, 0.00015)
    c = sample2d(curl, cuv)
    curl_vec = np.stack([c[..., 0] * 2 - 1, np.zeros_like(c[..., 0]), c[..., 1] * 2 - 1], axis=-1)
    curl_m = curl_vec * CURL_STRENGTH * 120.0
    curl_mag = np.clip(np.linalg.norm(curl_m, axis=-1) / 120.0, 0, 1)
    detail_pos = world * DETAIL_SCALE + curl_m * DETAIL_SCALE
    d = sample3d(detail, detail_pos)
    detail_n = np.clip(d[..., 0] * 0.625 + d[..., 1] * 0.25 + d[..., 2] * 0.125, 0, 1)

    edge = np.clip(1.0 - base_after, 0, 1)
    carve = (1.0 - detail_n) * EROSION * edge
    eroded = remap_clamped(base_after, carve, 1.0)
    erosion_delta = np.clip(base_after - eroded, 0, 1)

    bottom_region = 1.0 - bottom_fade
    wisp_carve = np.clip(1.0 - detail_n, 0, 1) * bottom_region * WISP
    bottom_wisps = np.clip(wisp_carve * eroded, 0, 1)
    final_d = remap_clamped(eroded, wisp_carve * 0.45, 1.0)
    final_d = np.where(final_d < 0.03, 0.0, final_d)

    stages = {
        "01_texture1_R": tex1_r,
        "02_perlin_worley_base": pw,
        "03_height_profile": np.full_like(tex1_r, float(np.mean(height))),
        "04_coverage_amount": cov,
        "05_base_after_height_coverage_bottom": base_after,
        "06_texture2_detail": detail_n,
        "07_erosion_delta": erosion_delta,
        "08_bottom_wisps": bottom_wisps,
        "09_curl_magnitude": curl_mag,
        "10_final_density": final_d,
        "11_precipitation": precip,
        "12_weather_type": type_sig,
        "90_weathermap_R_alone": cov,
        "91_weathermap_RGB": None,  # filled below
    }

    # Height is spatially varying via type weights
    stages["03_height_profile"] = height

    rgb = np.stack([cov, precip, type_sig], axis=-1)
    save_gray(OUT / "91_weathermap_R.png", cov)
    Image.fromarray(np.clip(np.rint(rgb * 255), 0, 255).astype(np.uint8), mode="RGB").save(
        OUT / "91_weathermap_RGB.png"
    )

    for name, arr in stages.items():
        if arr is None:
            continue
        if name.startswith("9"):
            continue
        save_gray(OUT / f"{name}.png", arr)

    # Validation: WeatherMap alone must not look like circular cloud footprints.
    # Proxy: high circularity of binary components would be bad; soft Perlin fields
    # have low component circularity vs hard Worley islands.
    mask = cov > 0.55
    # Count components roughly
    from collections import deque

    visited = np.zeros_like(mask, dtype=bool)
    n_cc = 0
    circ_scores = []
    h, w = mask.shape
    for y in range(h):
        for x in range(w):
            if not mask[y, x] or visited[y, x]:
                continue
            n_cc += 1
            q = deque([(y, x)])
            visited[y, x] = True
            cells = []
            while q:
                cy, cx = q.popleft()
                cells.append((cy, cx))
                for dy, dx in ((0, 1), (0, -1), (1, 0), (-1, 0)):
                    ny, nx = cy + dy, cx + dx
                    if 0 <= ny < h and 0 <= nx < w and mask[ny, nx] and not visited[ny, nx]:
                        visited[ny, nx] = True
                        q.append((ny, nx))
            if len(cells) < 20:
                continue
            ys = np.array([c[0] for c in cells], dtype=np.float64)
            xs = np.array([c[1] for c in cells], dtype=np.float64)
            area = float(len(cells))
            # Perimeter approx via neighbor count
            perim = 0.0
            cellset = set(cells)
            for cy, cx in cells:
                for dy, dx in ((0, 1), (0, -1), (1, 0), (-1, 0)):
                    if (cy + dy, cx + dx) not in cellset:
                        perim += 1.0
            # Circularity = 4πA / P² → 1 for circle
            if perim > 1:
                circ_scores.append(4.0 * np.pi * area / (perim * perim))

    mean_circ = float(np.mean(circ_scores)) if circ_scores else 0.0
    weather_not_circles = mean_circ < 0.55  # soft fields stay well below discs

    lines = [
        "=" * 72,
        "HZD 2015 density pipeline — offline stage audit",
        "=" * 72,
        "",
        "REMOVED / ABSENT:",
        "  FormationField, FormationMask, FormationScale, FormationCluster, FormationType",
        "  cloud islands, sphere/ellipsoid generators, custom size/cluster generators",
        "  anisotropic / domain-warp / FBM formation, WeatherMap-as-geometry",
        "  custom perlin_worley_dilate morphology, billowStrength dead uniform",
        "  worley2d island carve helper, CloudShearXZ density warping",
        "",
        "REMAINS (HZD-required only):",
        "  Texture1 128^3 (R=Perlin-Worley Remap, G/B/A=Worley increasing freq)",
        "  Texture2 32^3 Worley detail",
        "  Curl 128^2 detail distortion",
        "  WeatherMap R/G/B = coverage / precip / type (weather only)",
        "  ONE SampleCloudDensity path (VolumetricClouds.hlsli)",
        "  Height presets St/Sc/Cu/Cb + coverage Remap + bottom reduce",
        "  Texture2 edge erosion + bottom wisps + curl on detail UVs",
        "",
        "Stage means @ AGL {:.0f} m, {:.0f} m extent:".format(Y_AGL, EXTENT_M),
    ]
    for name, arr in stages.items():
        if arr is None or name.startswith("9"):
            continue
        lines.append(
            f"  {name:48s} mean={float(arr.mean()):.4f}  std={float(arr.std()):.4f}"
        )
    lines += [
        "",
        f"WeatherMap circularity proxy (mean 4πA/P² of cov>0.55 CCs): {mean_circ:.3f}",
        f"  [{'PASS' if weather_not_circles else 'FAIL'}] WeatherMap is NOT circular cloud geometry"
        f" (need < 0.55, got {mean_circ:.3f})",
        f"Texture1 supplies 3D base: mean(pw)={float(pw.mean()):.4f} std={float(pw.std()):.4f}",
        f"Texture2 supplies detail:  mean={float(detail_n.mean()):.4f}",
        f"Curl supplies turbulence:  mean={float(curl_mag.mean()):.4f}",
        f"Final density fully 3D-driven (Texture1×height×cov then Texture2): mean={float(final_d.mean()):.4f}",
        "",
        f"Slices: {OUT}",
        "=" * 72,
    ]
    text = "\n".join(lines) + "\n"
    REPORT.write_text(text, encoding="utf-8")
    print(text.encode("ascii", errors="replace").decode("ascii"))
    return 0 if weather_not_circles else 1


if __name__ == "__main__":
    sys.exit(main())
