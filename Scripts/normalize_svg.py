#!/usr/bin/env python3
"""Normalize editor SVG icons for pixel-perfect rasterization (nanosvg-compatible).

Fixes common Inkscape export issues:
  - offsets in viewBox (e.g. viewBox="13 24 231 203")
  - group transforms not baked into path data
  - noisy fractional coordinates

Usage:
  python Scripts/normalize_svg.py Assets/Editor/Folder.svg
  python Scripts/normalize_svg.py Assets/Editor/*.svg --in-place
  python Scripts/normalize_svg.py Assets/Editor --in-place --round-canvas
  python Scripts/normalize_svg.py icon.svg --canvas 256 --dry-run
"""

from __future__ import annotations

import argparse
import math
import re
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Iterator, Sequence

SVG_NS = "http://www.w3.org/2000/svg"
ET.register_namespace("", SVG_NS)

TOKEN_RE = re.compile(r"[a-zA-Z]|-?\d*\.?\d+(?:e[-+]?\d+)?", re.IGNORECASE)
TRANSFORM_RE = re.compile(
    r"(matrix|translate|scale|rotate|skewX|skewY)\s*"
    r"\(([^)]*)\)",
    re.IGNORECASE,
)


@dataclass(frozen=True)
class Matrix:
    a: float = 1.0
    b: float = 0.0
    c: float = 0.0
    d: float = 1.0
    e: float = 0.0
    f: float = 0.0

    def multiply(self, other: Matrix) -> Matrix:
        return Matrix(
            a=self.a * other.a + self.c * other.b,
            b=self.b * other.a + self.d * other.b,
            c=self.a * other.c + self.c * other.d,
            d=self.b * other.c + self.d * other.d,
            e=self.a * other.e + self.c * other.f + self.e,
            f=self.b * other.e + self.d * other.f + self.f,
        )

    def apply(self, x: float, y: float) -> tuple[float, float]:
        return (
            self.a * x + self.c * y + self.e,
            self.b * x + self.d * y + self.f,
        )


@dataclass
class PathState:
    x: float = 0.0
    y: float = 0.0
    sx: float = 0.0
    sy: float = 0.0
    last_cx: float | None = None
    last_cy: float | None = None
    last_qx: float | None = None
    last_qy: float | None = None


def parse_numbers(raw: str) -> list[float]:
    return [float(v) for v in re.split(r"[\s,]+", raw.strip()) if v]


def parse_transform(transform: str | None) -> Matrix:
    if not transform:
        return Matrix()

    matrix = Matrix()
    for match in TRANSFORM_RE.finditer(transform):
        kind = match.group(1).lower()
        values = parse_numbers(match.group(2))
        if kind == "matrix":
            if len(values) != 6:
                raise ValueError(f"matrix() expects 6 values, got {len(values)}")
            op = Matrix(*values)
        elif kind == "translate":
            tx = values[0] if values else 0.0
            ty = values[1] if len(values) > 1 else 0.0
            op = Matrix(e=tx, f=ty)
        elif kind == "scale":
            sx = values[0] if values else 1.0
            sy = values[1] if len(values) > 1 else sx
            op = Matrix(a=sx, d=sy)
        elif kind == "rotate":
            angle = math.radians(values[0] if values else 0.0)
            cos_a = math.cos(angle)
            sin_a = math.sin(angle)
            op = Matrix(a=cos_a, b=sin_a, c=-sin_a, d=cos_a)
            if len(values) >= 3:
                cx, cy = values[1], values[2]
                op = Matrix(e=cx, f=cy).multiply(op).multiply(Matrix(e=-cx, f=-cy))
        elif kind in {"skewx", "skewy"}:
            angle = math.radians(values[0] if values else 0.0)
            if kind == "skewx":
                op = Matrix(a=1.0, c=math.tan(angle))
            else:
                op = Matrix(b=math.tan(angle), d=1.0)
        else:
            raise ValueError(f"Unsupported transform: {kind}")
        matrix = matrix.multiply(op)
    return matrix


def tokenize_path(path_data: str) -> list[str]:
    return TOKEN_RE.findall(path_data)


def iter_commands(path_data: str) -> Iterator[tuple[str, list[float]]]:
    tokens = tokenize_path(path_data)
    index = 0
    while index < len(tokens):
        if not tokens[index].isalpha():
            raise ValueError(f"Expected path command, found {tokens[index]!r}")
        command = tokens[index]
        index += 1
        args: list[float] = []
        while index < len(tokens) and not tokens[index].isalpha():
            args.append(float(tokens[index]))
            index += 1
        yield command, args


def reflect(px: float, py: float, x: float, y: float) -> tuple[float, float]:
    return (2.0 * x - px, 2.0 * y - py)


def path_to_absolute(path_data: str) -> tuple[list[tuple[str, list[float]]], list[tuple[float, float]]]:
    state = PathState()
    commands: list[tuple[str, list[float]]] = []
    points: list[tuple[float, float]] = []

    for command, args in iter_commands(path_data):
        rel = command.islower()
        upper = command.upper()
        arg_index = 0

        if upper == "M":
            while arg_index < len(args):
                x = state.x + args[arg_index] if rel else args[arg_index]
                y = state.y + args[arg_index + 1] if rel else args[arg_index + 1]
                state.x, state.y = x, y
                state.sx, state.sy = x, y
                commands.append(("M", [x, y]))
                points.append((x, y))
                arg_index += 2
                upper = "L"
                rel = False
        elif upper == "L":
            while arg_index < len(args):
                x = state.x + args[arg_index] if rel else args[arg_index]
                y = state.y + args[arg_index + 1] if rel else args[arg_index + 1]
                state.x, state.y = x, y
                commands.append(("L", [x, y]))
                points.append((x, y))
                arg_index += 2
        elif upper == "H":
            while arg_index < len(args):
                x = state.x + args[arg_index] if rel else args[arg_index]
                state.x = x
                commands.append(("L", [x, state.y]))
                points.append((x, state.y))
                arg_index += 1
        elif upper == "V":
            while arg_index < len(args):
                y = state.y + args[arg_index] if rel else args[arg_index]
                state.y = y
                commands.append(("L", [state.x, y]))
                points.append((state.x, y))
                arg_index += 1
        elif upper == "C":
            while arg_index < len(args):
                x1 = state.x + args[arg_index] if rel else args[arg_index]
                y1 = state.y + args[arg_index + 1] if rel else args[arg_index + 1]
                x2 = state.x + args[arg_index + 2] if rel else args[arg_index + 2]
                y2 = state.y + args[arg_index + 3] if rel else args[arg_index + 3]
                x = state.x + args[arg_index + 4] if rel else args[arg_index + 4]
                y = state.y + args[arg_index + 5] if rel else args[arg_index + 5]
                commands.append(("C", [x1, y1, x2, y2, x, y]))
                points.extend([(x1, y1), (x2, y2), (x, y)])
                state.last_cx, state.last_cy = x2, y2
                state.last_qx = state.last_qy = None
                state.x, state.y = x, y
                arg_index += 6
        elif upper == "S":
            while arg_index < len(args):
                if state.last_cx is None or state.last_cy is None:
                    x1, y1 = state.x, state.y
                else:
                    x1, y1 = reflect(state.last_cx, state.last_cy, state.x, state.y)
                x2 = state.x + args[arg_index] if rel else args[arg_index]
                y2 = state.y + args[arg_index + 1] if rel else args[arg_index + 1]
                x = state.x + args[arg_index + 2] if rel else args[arg_index + 2]
                y = state.y + args[arg_index + 3] if rel else args[arg_index + 3]
                commands.append(("C", [x1, y1, x2, y2, x, y]))
                points.extend([(x1, y1), (x2, y2), (x, y)])
                state.last_cx, state.last_cy = x2, y2
                state.x, state.y = x, y
                arg_index += 4
        elif upper == "Q":
            while arg_index < len(args):
                x1 = state.x + args[arg_index] if rel else args[arg_index]
                y1 = state.y + args[arg_index + 1] if rel else args[arg_index + 1]
                x = state.x + args[arg_index + 2] if rel else args[arg_index + 2]
                y = state.y + args[arg_index + 3] if rel else args[arg_index + 3]
                commands.append(("Q", [x1, y1, x, y]))
                points.extend([(x1, y1), (x, y)])
                state.last_qx, state.last_qy = x1, y1
                state.last_cx = state.last_cy = None
                state.x, state.y = x, y
                arg_index += 4
        elif upper == "T":
            while arg_index < len(args):
                if state.last_qx is None or state.last_qy is None:
                    x1, y1 = state.x, state.y
                else:
                    x1, y1 = reflect(state.last_qx, state.last_qy, state.x, state.y)
                x = state.x + args[arg_index] if rel else args[arg_index]
                y = state.y + args[arg_index + 1] if rel else args[arg_index + 1]
                commands.append(("Q", [x1, y1, x, y]))
                points.extend([(x1, y1), (x, y)])
                state.last_qx, state.last_qy = x1, y1
                state.x, state.y = x, y
                arg_index += 2
        elif upper == "A":
            raise ValueError("Arc (A/a) commands are not supported; convert to curves in Inkscape first.")
        elif upper == "Z":
            commands.append(("Z", []))
            state.x, state.y = state.sx, state.sy
            state.last_cx = state.last_cy = None
            state.last_qx = state.last_qy = None
        else:
            raise ValueError(f"Unsupported path command: {command}")

    return commands, points


def transform_commands(commands: list[tuple[str, list[float]]], matrix: Matrix) -> list[tuple[str, list[float]]]:
    transformed: list[tuple[str, list[float]]] = []
    for command, args in commands:
        if command == "Z":
            transformed.append((command, []))
            continue
        if command in {"M", "L"}:
            x, y = matrix.apply(args[0], args[1])
            transformed.append((command, [x, y]))
        elif command == "C":
            x1, y1 = matrix.apply(args[0], args[1])
            x2, y2 = matrix.apply(args[2], args[3])
            x, y = matrix.apply(args[4], args[5])
            transformed.append((command, [x1, y1, x2, y2, x, y]))
        elif command == "Q":
            x1, y1 = matrix.apply(args[0], args[1])
            x, y = matrix.apply(args[2], args[3])
            transformed.append((command, [x1, y1, x, y]))
        else:
            raise ValueError(f"Cannot transform command {command}")
    return transformed


def fmt_number(value: float, decimals: int) -> str:
    rounded = round(value, decimals)
    if abs(rounded - round(rounded)) <= 10 ** -(decimals + 1):
        return str(int(round(rounded)))
    text = f"{rounded:.{decimals}f}".rstrip("0").rstrip(".")
    return text if text else "0"


def emit_path(commands: list[tuple[str, list[float]]], offset_x: float, offset_y: float, decimals: int) -> str:
    chunks: list[str] = []
    for command, args in commands:
        if command == "Z":
            chunks.append("Z")
            continue
        if command == "M":
            chunks.append(
                f"M{fmt_number(args[0] - offset_x, decimals)} {fmt_number(args[1] - offset_y, decimals)}"
            )
        elif command == "L":
            chunks.append(
                f"L{fmt_number(args[0] - offset_x, decimals)} {fmt_number(args[1] - offset_y, decimals)}"
            )
        elif command == "C":
            chunks.append(
                f"C{fmt_number(args[0] - offset_x, decimals)} {fmt_number(args[1] - offset_y, decimals)} "
                f"{fmt_number(args[2] - offset_x, decimals)} {fmt_number(args[3] - offset_y, decimals)} "
                f"{fmt_number(args[4] - offset_x, decimals)} {fmt_number(args[5] - offset_y, decimals)}"
            )
        elif command == "Q":
            chunks.append(
                f"Q{fmt_number(args[0] - offset_x, decimals)} {fmt_number(args[1] - offset_y, decimals)} "
                f"{fmt_number(args[2] - offset_x, decimals)} {fmt_number(args[3] - offset_y, decimals)}"
            )
    return "".join(chunks)


@dataclass
class PathShape:
    commands: list[tuple[str, list[float]]]
    points: list[tuple[float, float]]
    fill: str
    stroke: str | None
    stroke_width: str | None


def local_name(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def collect_paths(element: ET.Element, parent_matrix: Matrix) -> list[PathShape]:
    matrix = parent_matrix
    transform = element.attrib.get("transform")
    if transform:
        matrix = parent_matrix.multiply(parse_transform(transform))

    shapes: list[PathShape] = []
    tag = local_name(element.tag)
    if tag == "path" and element.attrib.get("d"):
        commands, points = path_to_absolute(element.attrib["d"])
        commands = transform_commands(commands, matrix)
        points = [matrix.apply(x, y) for x, y in points]
        style = element.attrib.get("style", "")
        fill = element.attrib.get("fill")
        if not fill and "fill:" in style:
            fill_match = re.search(r"fill:\s*([^;]+)", style)
            fill = fill_match.group(1).strip() if fill_match else None
        fill = fill or "#000000"
        if fill.lower() in {"none", "transparent"}:
            fill = "none"
        stroke = element.attrib.get("stroke")
        stroke_width = element.attrib.get("stroke-width")
        shapes.append(PathShape(commands, points, fill, stroke, stroke_width))
    else:
        for child in list(element):
            shapes.extend(collect_paths(child, matrix))
    return shapes


def compute_bounds(shapes: Sequence[PathShape]) -> tuple[float, float, float, float]:
    xs = [x for shape in shapes for x, _ in shape.points]
    ys = [y for shape in shapes for _, y in shape.points]
    if not xs or not ys:
        raise ValueError("No drawable geometry found in SVG.")
    return min(xs), min(ys), max(xs), max(ys)


def ceil_canvas(value: float) -> int:
    return max(1, int(math.ceil(value - 1e-6)))


def build_svg(
    shapes: Sequence[PathShape],
    width: int,
    height: int,
    decimals: int,
    offset_x: float,
    offset_y: float,
) -> str:
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<svg xmlns="{SVG_NS}" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}" fill="none" shape-rendering="geometricPrecision">',
    ]
    for shape in shapes:
        attrs = [f'fill="{shape.fill}"']
        if shape.stroke and shape.stroke.lower() != "none":
            attrs.append(f'stroke="{shape.stroke}"')
        if shape.stroke_width:
            attrs.append(f'stroke-width="{shape.stroke_width}"')
        path_data = emit_path(shape.commands, offset_x, offset_y, decimals)
        lines.append(f'  <path {" ".join(attrs)} d="{path_data}"/>')
    lines.append("</svg>")
    lines.append("")
    return "\n".join(lines)


@dataclass
class NormalizeResult:
    source: Path
    width: int
    height: int
    aspect: float
    output: str


def normalize_svg_file(
    source: Path,
    *,
    decimals: int,
    round_canvas: bool,
    square_canvas: int | None,
    padding: float,
) -> NormalizeResult:
    tree = ET.parse(source)
    root = tree.getroot()
    if local_name(root.tag) != "svg":
        raise ValueError(f"{source} root element is not <svg>.")

    shapes = collect_paths(root, Matrix())
    min_x, min_y, max_x, max_y = compute_bounds(shapes)
    content_w = max_x - min_x
    content_h = max_y - min_y

    offset_x = min_x - padding
    offset_y = min_y - padding
    width_f = content_w + padding * 2.0
    height_f = content_h + padding * 2.0

    if square_canvas is not None:
        width = height = square_canvas
        # Center content inside square canvas by expanding offsets.
        pad_x = (square_canvas - width_f) * 0.5
        pad_y = (square_canvas - height_f) * 0.5
        if pad_x > 0:
            offset_x -= pad_x
            width_f = square_canvas
        if pad_y > 0:
            offset_y -= pad_y
            height_f = square_canvas
        width = height = square_canvas
    elif round_canvas:
        width = ceil_canvas(width_f)
        height = ceil_canvas(height_f)
    else:
        width = ceil_canvas(width_f)
        height = ceil_canvas(height_f)

    output = build_svg(shapes, width, height, decimals, offset_x, offset_y)
    aspect = width / height if height else 1.0
    return NormalizeResult(source=source, width=width, height=height, aspect=aspect, output=output)


def expand_inputs(inputs: Sequence[str]) -> list[Path]:
    paths: list[Path] = []
    for item in inputs:
        path = Path(item)
        if path.is_dir():
            paths.extend(sorted(path.glob("*.svg")))
        elif path.is_file():
            paths.append(path)
        else:
            matches = sorted(Path().glob(item))
            if not matches:
                raise FileNotFoundError(f"No SVG files matched: {item}")
            paths.extend(match for match in matches if match.is_file())
    return paths


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("inputs", nargs="+", help="SVG files, directories, or glob patterns")
    parser.add_argument("--in-place", action="store_true", help="Overwrite source SVG files")
    parser.add_argument("--output", type=Path, help="Write a single normalized SVG to this path")
    parser.add_argument("--decimals", type=int, default=2, help="Coordinate decimal places (default: 2)")
    parser.add_argument("--padding", type=float, default=0.0, help="Padding around content in user units")
    parser.add_argument("--round-canvas", action="store_true", help="Ceil viewBox width/height to integers")
    parser.add_argument("--canvas", type=int, help="Fit content into a square canvas of this size (e.g. 256)")
    parser.add_argument("--dry-run", action="store_true", help="Print summary without writing files")
    args = parser.parse_args(argv)

    if args.output and (args.in_place or len(args.inputs) != 1):
        parser.error("--output requires exactly one input and cannot be combined with --in-place")

    try:
        sources = expand_inputs(args.inputs)
    except FileNotFoundError as exc:
        print(exc, file=sys.stderr)
        return 1

    if not sources:
        print("No SVG files to process.", file=sys.stderr)
        return 1

    exit_code = 0
    for source in sources:
        try:
            result = normalize_svg_file(
                source,
                decimals=args.decimals,
                round_canvas=args.round_canvas or not args.canvas,
                square_canvas=args.canvas,
                padding=args.padding,
            )
        except Exception as exc:  # noqa: BLE001 - CLI tool reports per-file failures
            print(f"[error] {source}: {exc}", file=sys.stderr)
            exit_code = 1
            continue

        summary = (
            f"{source}: {result.width}x{result.height} "
            f"(aspect {result.aspect:.3f} = {result.width}/{result.height})"
        )
        print(summary)

        if args.dry_run:
            continue

        destination = args.output if args.output else source
        if not args.in_place and not args.output:
            destination = source.with_name(f"{source.stem}.normalized{source.suffix}")
            print(f"  -> {destination}")

        destination.write_text(result.output, encoding="utf-8", newline="\n")

    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
