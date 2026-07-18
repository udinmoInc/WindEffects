from pathlib import Path

ROOT = Path(".")

EXTENSIONS = {
    ".cpp": "C++",
    ".cc": "C++",
    ".cxx": "C++",
    ".h": "C/C++ Header",
    ".hpp": "C/C++ Header",
    ".hh": "C/C++ Header",
    ".hlsl": "HLSL",
    ".glsl": "GLSL",
    ".c": "C",
    ".cs": "C#",
    ".py": "Python",
    ".ps1": "PowerShell",
    ".bat": "Batch",
    ".cmake": "CMake",
    ".json": "JSON",
    ".xml": "XML",
}

EXCLUDE_NAMES = {
    ".git",
    ".vs",
    ".idea",
    ".vscode",
    "bin",
    "obj",
    "Build",
    "Binaries",
    "Intermediate",
    "DerivedDataCache",
    "Saved",
    "Cache",
    "Temp",
    "Logs",
    "Assets",
    "Content",
    "Textures",
    "Models",
    "Audio",
    "Fonts",
    "ShadersCache",
    "Generated",
    "Packages",
}

THIRDPARTY_HINTS = {
    "ThirdParty",
    "thirdparty",
    "external",
    "extern",
    "vendor",
    "vendors",
    "deps",
    "dependencies",
    "vcpkg",
    "conan",
    "submodules",
}

stats = {}

for file in ROOT.rglob("*"):

    if not file.is_file():
        continue

    parts = {p.lower() for p in file.parts}

    if any(x.lower() in parts for x in EXCLUDE_NAMES):
        continue

    if any(x.lower() in parts for x in THIRDPARTY_HINTS):
        continue

    ext = file.suffix.lower()

    if ext not in EXTENSIONS:
        continue

    try:
        lines = sum(1 for _ in open(file, "r", encoding="utf-8", errors="ignore"))
    except:
        continue

    lang = EXTENSIONS[ext]

    if lang not in stats:
        stats[lang] = [0, 0]

    stats[lang][0] += 1
    stats[lang][1] += lines

total_files = 0
total_lines = 0

print("=" * 60)
print("WindEffects Code Statistics")
print("=" * 60)

for lang in sorted(stats):
    files, lines = stats[lang]
    total_files += files
    total_lines += lines
    print(f"{lang:18} {files:6} files {lines:12,} lines")

print("-" * 60)
print(f"TOTAL FILES : {total_files:,}")
print(f"TOTAL LINES : {total_lines:,}")