// ==============================================================================
// WindEffects — IgniteBT — ExactChangeClassifier
// Source file for the IgniteBT module.
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.Build.Graph;
using IgniteBT.Workspace.Modules;

namespace IgniteBT.Build.Dependencies;

public enum ChangeClassification
{
    CPP_IMPLEMENTATION,
    PRIVATE_HEADER,
    PUBLIC_HEADER,
    PCH_HEADER,
    GENERATED_HEADER,
    MODULE_DEFINITION,
    BUILD_CONFIGURATION,
    COMPILER_CONFIGURATION,
    ASSET_ONLY,
    UNRELATED
}

public sealed class FileChangeDetail
{
    public string FilePath { get; init; } = string.Empty;
    public ChangeClassification Classification { get; init; }
    public string AssociatedModule { get; init; } = string.Empty;
    public bool InvalidatesWholeModule { get; init; }
    public bool RequiresPchRebuild { get; init; }
}

public static class ExactChangeClassifier
{
    private static readonly HashSet<string> CppExtensions = new(StringComparer.OrdinalIgnoreCase)
    {
        ".cpp", ".cxx", ".cc", ".c"
    };

    private static readonly HashSet<string> HeaderExtensions = new(StringComparer.OrdinalIgnoreCase)
    {
        ".h", ".hpp", ".inl", ".hh"
    };

    private static readonly HashSet<string> AssetExtensions = new(StringComparer.OrdinalIgnoreCase)
    {
        ".png", ".jpg", ".tga", ".wav", ".mp3", ".hlsl", ".glsl", ".json", ".xml", ".txt", ".ico"
    };

    public static FileChangeDetail Classify(string filePath, DiscoveredModule? module, DependencyGraph? graph)
    {
        var ext = Path.GetExtension(filePath);
        var fileName = Path.GetFileName(filePath);
        var moduleName = module?.Name ?? string.Empty;

        // 1. Build & Module definition files
        if (fileName.EndsWith(".Build.cs", StringComparison.OrdinalIgnoreCase) ||
            fileName.EndsWith(".Target.cs", StringComparison.OrdinalIgnoreCase))
        {
            return new FileChangeDetail
            {
                FilePath = filePath,
                Classification = ChangeClassification.MODULE_DEFINITION,
                AssociatedModule = moduleName,
                InvalidatesWholeModule = true,
                RequiresPchRebuild = true
            };
        }

        // 2. Precompiled headers
        if (fileName.Contains("PCH", StringComparison.OrdinalIgnoreCase) ||
            (module != null && !string.IsNullOrEmpty(module.PrecompiledHeader) &&
             filePath.EndsWith(module.PrecompiledHeader, StringComparison.OrdinalIgnoreCase)))
        {
            return new FileChangeDetail
            {
                FilePath = filePath,
                Classification = ChangeClassification.PCH_HEADER,
                AssociatedModule = moduleName,
                InvalidatesWholeModule = false,
                RequiresPchRebuild = true
            };
        }

        // 3. Generated headers/sources
        if (fileName.EndsWith(".gen.h", StringComparison.OrdinalIgnoreCase) ||
            fileName.EndsWith(".generated.h", StringComparison.OrdinalIgnoreCase) ||
            filePath.Contains("Intermediate", StringComparison.OrdinalIgnoreCase))
        {
            return new FileChangeDetail
            {
                FilePath = filePath,
                Classification = ChangeClassification.GENERATED_HEADER,
                AssociatedModule = moduleName,
                InvalidatesWholeModule = false,
                RequiresPchRebuild = false
            };
        }

        // 4. C++ Implementation files (.cpp, .cxx)
        if (CppExtensions.Contains(ext))
        {
            return new FileChangeDetail
            {
                FilePath = filePath,
                Classification = ChangeClassification.CPP_IMPLEMENTATION,
                AssociatedModule = moduleName,
                InvalidatesWholeModule = false,
                RequiresPchRebuild = false
            };
        }

        // 5. Headers (.h, .hpp, .inl)
        if (HeaderExtensions.Contains(ext))
        {
            bool isPublic = IsPublicHeader(filePath, module);
            return new FileChangeDetail
            {
                FilePath = filePath,
                Classification = isPublic ? ChangeClassification.PUBLIC_HEADER : ChangeClassification.PRIVATE_HEADER,
                AssociatedModule = moduleName,
                InvalidatesWholeModule = false,
                RequiresPchRebuild = false
            };
        }

        // 6. Non-code asset files
        if (AssetExtensions.Contains(ext))
        {
            return new FileChangeDetail
            {
                FilePath = filePath,
                Classification = ChangeClassification.ASSET_ONLY,
                AssociatedModule = moduleName,
                InvalidatesWholeModule = false,
                RequiresPchRebuild = false
            };
        }

        // 7. Unrelated workspace files (.md, .git, etc.)
        return new FileChangeDetail
        {
            FilePath = filePath,
            Classification = ChangeClassification.UNRELATED,
            AssociatedModule = moduleName,
            InvalidatesWholeModule = false,
            RequiresPchRebuild = false
        };
    }

    private static bool IsPublicHeader(string filePath, DiscoveredModule? module)
    {
        if (module == null) return false;
        var normalized = filePath.Replace('\\', '/');
        if (normalized.Contains("/Public/", StringComparison.OrdinalIgnoreCase)) return true;
        return module.PublicIncludePaths.Any(p => filePath.StartsWith(p, StringComparison.OrdinalIgnoreCase));
    }
}
