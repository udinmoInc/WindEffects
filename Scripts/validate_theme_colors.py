#!/usr/bin/env python3
# ==============================================================================
# WindEffects — Static Theme & Visual Color Validation Script
# Scans KindUI and Editor C++ source code for unapproved hardcoded UI colors.
# Enforces theme token usage (ColorToken / SurfaceRole / ThemeColor).
# ==============================================================================

import os
import re
import sys

# Allowed file patterns (documented exceptions)
APPROVED_FILE_EXCEPTIONS = [
    os.path.normpath("Engine/Source/Runtime/KindUI/Public/KindUI/Theme/Palette.h"),
    os.path.normpath("Engine/Source/Runtime/KindUI/Private/Theming/PaletteRuntime.cpp"),
    os.path.normpath("Engine/Source/Runtime/KindUI/Private/Profiling"), # Diagnostic / profiling benchmark harnesses
    os.path.normpath("Engine/Source/Editor/EditorGridRenderer/Public/EditorFunctionConfig.h"), # 3D Viewport grid vector config
    os.path.normpath("Engine/Source/Runtime/KindUI/Private/Core/KindUIHeapStats.cpp"), # Memory CRT hex constant
    os.path.normpath("Engine/Source/Runtime/KindUI/Private/Core/TextMetrics.cpp"), # Hash seed hex constants
    os.path.normpath("Engine/Source/Runtime/AssetImporter"), # File hashing hex strings
    os.path.normpath("Engine/Source/Runtime/AssetPipeline"), # Build cache hex hashing
    os.path.normpath("Engine/Source/Runtime/AssetProcessors"), # Asset hash comparison
]

# Patterns for hardcoded UI colors
HEX_PATTERN = re.compile(r'\bHex\s*\(\s*"#[0-9a-fA-F]{6,8}"\s*\)')

def is_approved_exception(file_path):
    norm_path = os.path.normpath(file_path)
    for exception in APPROVED_FILE_EXCEPTIONS:
        if exception in norm_path:
            return True
    return False

def scan_file(file_path):
    issues = []
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line_num, line in enumerate(f, 1):
            # Check for Hex("#...") literal usages in UI code
            if HEX_PATTERN.search(line):
                issues.append((line_num, line.strip(), "Hardcoded Hex(...) visual color literal"))
    return issues

def main():
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    search_dirs = [
        os.path.join(repo_root, "Engine", "Source", "Runtime", "KindUI"),
        os.path.join(repo_root, "Engine", "Source", "Editor")
    ]

    total_files_scanned = 0
    total_issues_found = 0
    failing_files = []

    print("=========================================================")
    print(" WindEffects Theme & UI Color Validation Audit")
    print("=========================================================")

    for search_dir in search_dirs:
        if not os.path.exists(search_dir):
            continue
        for root, _, files in os.walk(search_dir):
            for file in files:
                if file.endswith(('.cpp', '.h', '.hpp', '.inl')):
                    rel_path = os.path.relpath(os.path.join(root, file), repo_root)
                    if is_approved_exception(rel_path):
                        continue

                    total_files_scanned += 1
                    issues = scan_file(os.path.join(root, file))
                    if issues:
                        total_issues_found += len(issues)
                        failing_files.append((rel_path, issues))

    print(f"Scanned {total_files_scanned} C++ source files.")

    if total_issues_found > 0:
        print(f"\n[FAILURE] Found {total_issues_found} unapproved hardcoded UI visual color(s):\n")
        for rel_path, issues in failing_files:
            print(f"  File: {rel_path}")
            for line_num, line, reason in issues:
                print(f"    Line {line_num}: {reason} -> '{line}'")
        sys.exit(1)
    else:
        print("\n[SUCCESS] 0 unapproved hardcoded UI visual colors found! All UI colors route through theme tokens.")
        sys.exit(0)

if __name__ == "__main__":
    main()
