#!/usr/bin/env python3
"""One-shot migration helper: replace legacy icon API patterns with WindIcons."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "Engine" / "Source"

REPLACEMENTS = [
    (r'#include "KindUI/Core/KindIcons.h"\n', ''),
    (r'#include "KindUI/Core/Icon.h"\n', '#include "KindUI/Core/WindIcon.h"\n#include "KindUI/Core/Icon.h"\n'),
    (r'namespace Icons = ::we::runtime::kindui::Icons;\n', 'using ::we::runtime::kindui::WindIcons;\nusing ::we::runtime::kindui::kWindIconNone;\n'),
    (r'Icons::SearchName', 'WindIcons::Search16'),
    (r'Icons::XName', 'WindIcons::Close16'),
    (r'Icons::ChevronDownName', 'WindIcons::ChevronDown16'),
    (r'Icons::ChevronRightName', 'WindIcons::ChevronRight16'),
    (r'Icons::ChevronLeftName', 'WindIcons::ChevronLeft16'),
    (r'Icons::ChevronUpName', 'WindIcons::ChevronUp16'),
    (r'Icons::UndoName', 'WindIcons::Undo16'),
    (r'Icons::RedoName', 'WindIcons::Redo16'),
    (r'Icons::RefreshName', 'WindIcons::Refresh16'),
    (r'Icons::CheckName', 'WindIcons::Check16'),
    (r'Icons::MinusName', 'WindIcons::Minus16'),
    (r'Icons::EyeName', 'WindIcons::Eye16'),
    (r'Icons::SunName', 'WindIcons::Sun16'),
    (r'Icons::GlobeName', 'WindIcons::Globe16'),
    (r'Icons::GridName', 'WindIcons::Grid3x316'),
    (r'Icons::FilterName', 'WindIcons::ListFilter16'),
    (r'Icons::MoreName', 'WindIcons::VerticalDots16'),
    (r'Icons::WrenchName', 'WindIcons::Wrench16'),
    (r'Icons::BuildName', 'WindIcons::Wrench16'),
    (r'Icons::ScalingName', 'WindIcons::Scaling16'),
    (r'KindIcons::Search', 'WindIcons::Search16'),
    (r'KindIcons::Settings', 'kWindIconNone'),
    (r'KindIcons::Star', 'kWindIconNone'),
    (r'KindIcons::Object', 'kWindIconNone'),
    (r'KindIcons::Sun', 'WindIcons::Sun16'),
    (r'KindIcons::Globe', 'WindIcons::Globe16'),
    (r'KindIcons::ChevronDown', 'WindIcons::ChevronDown16'),
    (r'KindIcons::ChevronLeft', 'WindIcons::ChevronLeft16'),
    (r'KindIcons::Folder', 'kWindIconNone'),
    (r'KindIcons::FolderOpened', 'kWindIconNone'),
    (r'IconPainter::DrawCompactIcon\(([^,]+),\s*([^,]+),\s*([^,]+),\s*[^)]+\)', r'IconPainter::Draw(\1, \2, \3)'),
    (r'IconPainter::DrawIcon\(([^,]+),\s*([^,]+),\s*([^,]+),\s*[^)]+\)', r'IconPainter::Draw(\1, \2, \3)'),
    (r'IconMetrics::CompactGlyphTierPx\(\)', '16u'),
    (r'IconMetrics::StandardGlyphTierPx\(\)', '16u'),
    (r'IconMetrics::NativeIconTierPx\([^)]+\)', '16u'),
    (r'IconMetrics::TierPxForIcon\([^)]+\)', '16u'),
    (r'IconMetrics::GlyphTierPx\([^)]+\)', '16u'),
]

# Icons with no WindIcon equivalent -> blank
BLANK_ICONS = [
    'NewName', 'OpenName', 'OpenFolderName', 'FolderName', 'FolderAddName', 'SaveName', 'SaveAllName',
    'SettingsName', 'PlayName', 'PlaySolidName', 'PauseName', 'StopName', 'PlusName', 'StarName',
    'StarFilledName', 'ContentBrowserName', 'TerminalName', 'OutputLogName', 'HierarchyName',
    'PropertiesName', 'LayersName', 'PivotName', 'CameraName', 'LightName', 'CubeName', 'Cube3DName',
    'MediaPlayName', 'ProfilerName', 'DocumentName', 'PackageName', 'ProjectFolderName', 'InfoName',
    'WindLogoName', 'MaximizeName', 'MinimizeName', 'RestoreName', 'MonitorName', 'ToolbarEnvironmentName',
    'ToolbarObjectName', 'LockName', 'UnlockName', 'DeleteName', 'CopyName', 'PasteName', 'MenuName',
    'MountainName', 'Volume2Name', 'BlocksName', 'ComponentName', 'MapName', 'CodeName', 'VideoName',
    'PerspectiveName', 'MoveName', 'RotateName', 'ScaleName', 'WireframeName', 'SnapName', 'AddActorName',
    'NewFileName', 'RecentName', 'UserName', 'TreesName', 'ConeName', 'CapsuleName', 'FlashlightName',
    'BrainName', 'LayoutPanelName', 'StickyNoteName', 'CrosshairName', 'SparklesName', 'ZapName',
    'BuildName', 'ListName', 'EyeOffName', 'PinName', 'SuccessName', 'WarningName', 'ErrorName',
    'CompassName', 'EraserName', 'BrushName', 'RecordName', 'LitName', 'ShaderName', 'TextureName',
    'PlaneName', 'SphereName', 'CylinderName', 'Cone3DName', 'Capsule3DName', 'BlankActor3DName',
    'Sphere3DName', 'Plane3DName', 'Cylinder3DName',
]

for name in BLANK_ICONS:
    REPLACEMENTS.append((rf'Icons::{name}', 'kWindIconNone'))


def migrate_file(path: Path) -> bool:
    text = path.read_text(encoding='utf-8', errors='replace')
    original = text
    for pattern, repl in REPLACEMENTS:
        text = re.sub(pattern, repl, text)
    if text != original:
        path.write_text(text, encoding='utf-8')
        return True
    return False


def main() -> int:
    changed = []
    for path in ROOT.rglob('*'):
        if path.suffix not in {'.cpp', '.h'}:
            continue
        if 'ThumbnailRenderer' in str(path) or 'EngineIconArt' in str(path):
            continue
        if migrate_file(path):
            changed.append(path)
    print(f'Updated {len(changed)} files')
    for p in changed:
        print(p.relative_to(ROOT.parent.parent))
    return 0


if __name__ == '__main__':
    sys.exit(main())
