// ==============================================================================
// WindEffects — IgniteBT — VersionCommand
// Source file for the IgniteBT module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
namespace IgniteBT.CLI;

public static class VersionCommand
{
    public static int Execute()
    {
        Console.WriteLine("IgniteBT v1.0.0");
        Console.WriteLine("WindEffects Build Tool");
        Console.WriteLine(".NET 8.0");
        return 0;
    }
}
