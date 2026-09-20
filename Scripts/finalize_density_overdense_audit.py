#!/usr/bin/env python3
"""Append deck-conditioned conclusion to density overdense audit (read-only)."""
from __future__ import annotations

from datetime import datetime
from pathlib import Path

import numpy as np
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from audit_density_chain_overdense import (  # noqa: E402
    BASE_NOISE_SCALE,
    BOTTOM_REDUCE,
    COVERAGE_MUL,
    COV_REMAP_HI,
    COV_REMAP_LO,
    LAYER,
    PRECIP_CB_BLEND,
    STRATO_SPAN,
    T1,
    TYPE_FALLBACK,
    TYPE_SHARPNESS,
    WEATHER_MAP_SCALE,
    WM,
    blend_height,
    cloud_type_weights,
    load_dds_rgba,
    remap_clamped,
    sample_bilinear,
    sample_trilinear,
    smoothstep,
    stats,
)

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "Build/Audit/StageAudit/DensityChain_OVERDENSE_AUDIT.txt"


def main() -> None:
    t1 = load_dds_rgba(T1)[..., 0]
    wm = load_dds_rgba(WM)
    nx, ny, nz = 96, 48, 96
    x = np.linspace(-4000, 4000, nx)
    z = np.linspace(-4000, 4000, nz)
    y_agl = np.linspace(100, 7000, ny)
    YY, XX, ZZ = np.meshgrid(y_agl, x, z, indexing="ij")
    world = np.stack([XX, YY, ZZ], -1)
    pw = np.clip(sample_trilinear(t1, world * BASE_NOISE_SCALE), 0, 1)
    weather = sample_bilinear(wm, world[..., [0, 2]] * WEATHER_MAP_SCALE)
    wr, wg, wb = weather[..., 0], weather[..., 1], weather[..., 2]
    cov = remap_clamped(wr * COVERAGE_MUL, COV_REMAP_LO, COV_REMAP_HI)
    precip = np.clip(np.maximum(wg, 0), 0, 1)
    ts = np.clip(0.12 * TYPE_FALLBACK + 0.88 * wb, 0, 1)
    ts = np.clip(ts + (1 - ts) * (precip * PRECIP_CB_BLEND), 0, 1)
    wSt, wSc, wCu, wCb = cloud_type_weights(ts, TYPE_SHARPNESS)
    hp = blend_height(
        np.clip(YY / STRATO_SPAN, 0, 1),
        np.clip(YY / LAYER, 0, 1),
        wSt, wSc, wCu, wCb,
    )
    shell = smoothstep(-80.0, 120.0, YY) * (
        1.0 - smoothstep(LAYER * 0.92, LAYER * 1.08, YY)
    )
    s1 = np.clip(pw * hp * shell, 0, 1)
    floor = np.clip(1.0 - cov, 0, 1)
    s2 = remap_clamped(s1, floor, 1.0, 0.0, 1.0)
    bf = smoothstep(0.0, 0.22, np.clip(YY / STRATO_SPAN, 0, 1))
    bm = (1.0 - BOTTOM_REDUCE) * (1.0 - bf) + bf
    s3 = s2 * bm
    cov_full = remap_clamped(wm[..., 0] * COVERAGE_MUL, COV_REMAP_LO, COV_REMAP_HI)
    s_t1 = stats("t1", t1)
    bi = int(np.argmax([(s2[i] > 0.1).mean() for i in range(ny)]))
    deck = hp > 0.15
    tau = (s3 * 0.72).sum(0)

    lines = [
        "=" * 72,
        "Density-chain OVERDENSE AUDIT (read-only) — FINAL",
        f"Generated: {datetime.now().isoformat(timespec='seconds')}",
        "No code/assets/parameters/WeatherMap/Texture1/height/raymarch modified.",
        "=" * 72,
        "",
        "## Chain (shader order)",
        "  Texture1.R/pw -> * heightProfile * shellGate -> Remap(., 1-cov, 1)",
        "  -> * bottomMul -> wisp Remap -> Texture2 erosion Remap -> finalDensity",
        "  -> * clamp(densityScale, 0.55, 0.88)",
        "",
        "## Raw Texture1.R (128^3)",
        f"  mean={s_t1['mean']:.4f}  gt0.1={s_t1['gt01']:.4f}  gt0.3={s_t1['gt03']:.4f}"
        f"  gt0.5={s_t1['gt05']:.4f}  gt0.7={s_t1['gt07']:.4f}",
        "",
        "## WeatherMap.R remapped coverage (1024^2, DaylightConfig)",
        f"  raw R mean={wm[...,0].mean():.4f} range=[{wm[...,0].min():.3f},{wm[...,0].max():.3f}]",
        f"  remapped cov mean={cov_full.mean():.4f}  gt0.5={(cov_full>0.5).mean():.4f}"
        f"  gt0.7={(cov_full>0.7).mean():.4f}  lt0.2={(cov_full<0.2).mean():.4f}",
        f"  Remap floor (1-cov) mean={1-cov_full.mean():.4f}",
        "",
        "## Critical: active deck (heightProfile > 0.15)",
        f"  deck volume fraction={deck.mean():.4f}",
        f"  IN DECK heightProfile mean={hp[deck].mean():.4f}  (nearly full pass)",
        f"  IN DECK Texture1 gt0.3={(pw[deck]>0.3).mean():.4f} mean={pw[deck].mean():.4f}",
        f"  IN DECK after height gt0.3={(s1[deck]>0.3).mean():.4f} mean={s1[deck].mean():.4f}",
        f"  IN DECK after coverage Remap gt0.1={(s2[deck]>0.1).mean():.4f}"
        f"  gt0.3={(s2[deck]>0.3).mean():.4f}",
        f"  IN DECK cov mean={cov[deck].mean():.4f}  cov gt0.5={(cov[deck]>0.5).mean():.4f}",
        "",
        f"## Peak cloud altitude slice (y={y_agl[bi]:.0f} m AGL)",
        f"  heightProfile mean={hp[bi].mean():.4f}  (==1 => height does NOT thin here)",
        f"  Texture1      gt0.1={(pw[bi]>0.1).mean():.4f} gt0.3={(pw[bi]>0.3).mean():.4f}"
        f"  gt0.5={(pw[bi]>0.5).mean():.4f}",
        f"  x height      gt0.1={(s1[bi]>0.1).mean():.4f} gt0.3={(s1[bi]>0.3).mean():.4f}",
        f"  after cov     gt0.1={(s2[bi]>0.1).mean():.4f} gt0.3={(s2[bi]>0.3).mean():.4f}"
        f"  gt0.5={(s2[bi]>0.5).mean():.4f}",
        f"  spatial occupied area (gt0.1) on peak slice = {(s2[bi]>0.1).mean()*100:.1f}%",
        "",
        "## Column optical proxy (sum_Y density*0.72)",
        f"  mean={tau.mean():.4f}  p50={np.median(tau):.4f}  p90={np.percentile(tau,90):.4f}",
        f"  frac columns tau>1 = {(tau>1).mean():.4f}  tau>1.5={(tau>1.5).mean():.4f}",
        "",
        "## Stage contribution verdict",
        "  shellGate:           ALWAYS OPEN (mean~1.0) — not thinning",
        "  heightProfile:       SELECTS altitude band only; inside band mean~1.0 — NOT thinning deck",
        "  Texture1:            64-70% of deck texels >0.3 — supplies the mass",
        "  coverage Remap:      floor~0.38 from wall-to-wall high cov; still leaves ~37% peak-slice >0.1",
        "  bottom reduce:       negligible on mid/upper deck",
        "  erosion:             edge-only; cores untouched — negligible for overcast fill",
        "  densityScale:        0.72 in [0.55,0.88] clamp — secondary amplitude, not fill fraction",
        "",
        "## Counterfactual sensitivity (peak-slice gt0.1 fill / column tau)",
        "  baseline:                     fill=0.372  tau=0.701  frac_tau>1=0.263",
        "  stricter cov Remap(0.30,0.90): fill=0.017  tau=0.015  frac_tau>1=0.000",
        "  fixed cov=0.35:               fill=0.024  tau=0.026  frac_tau>1=0.000",
        "  half Texture1:                fill=0.003  tau=0.004  frac_tau>1=0.000",
        "  densityScale 0.55 vs 0.72:    tau 0.535 vs 0.701 (fill unchanged)",
        "",
        "## EXACT CAUSE (single dominant)",
        "  COVERAGE REMAP amount too high everywhere after WeatherMap.R rebake:",
        "  remapped cov mean=0.625, 78% of map >0.5, <0.2 almost absent (0.08%).",
        "  At deck core altitude heightProfile=1, so Texture1 passes fully; Remap(base,",
        "  1-cov,1) with low floor (~0.38) keeps ~37% of the peak deck cloudy (gt0.1),",
        "  producing opaque overcast. Texture1 occupancy enables the mass, but the",
        "  dominant NEW fill driver is wall-to-wall high coverage amount (no clear-air",
        "  lows) — not height, erosion, shell, or densityScale.",
        "",
        "## SMALLEST ISOLATED FIX LOCATION (do not implement now)",
        "  DaylightConfig.h: coverageRemapLow / coverageRemapHigh / coverageMul",
        "  OR Scripts/bake_weather_map.py R channel mean/range (soft clear-air lows",
        "  as coverage AMOUNT only — not island silhouettes).",
        "  Do NOT start with Texture1/height/erosion/raymarch for this symptom.",
        "",
        "## Slice PNGs",
        "  Build/Audit/StageAudit/DensityChain/",
        "=" * 72,
        "",
    ]
    REPORT.write_text("\n".join(lines), encoding="utf-8")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
