// ==============================================================================
// WindEffects — IgniteBT — PathNormalizer
// Source file for the IgniteBT module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using System.Collections.Concurrent;

namespace IgniteBT.Core.Filesystem;

/// <summary>
/// Path normalization cache — avoids repeated Path.GetFullPath and string allocations.
/// </summary>
public sealed class PathNormalizer
{
    private readonly ConcurrentDictionary<string, string> _cache = new(StringComparer.OrdinalIgnoreCase);

    public string Normalize(string path)
    {
        if (string.IsNullOrWhiteSpace(path)) return path;
        return _cache.GetOrAdd(path, static p =>
        {
            try { return Path.GetFullPath(p).Replace('\\', '/'); }
            catch { return p.Replace('\\', '/'); }
        });
    }

    public int CacheSize => _cache.Count;
}
