// ==============================================================================
// WindEffects — Icons — Icons.Build
// Source file for the Icons module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;
using System.IO;

public class Icons : ModuleRules
{
    public Icons(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");

        var thirdPartyRoot = Path.Combine(context.EngineDirectory, "ThirdParty");
        PrivateIncludePaths.Add(Path.Combine(thirdPartyRoot, "stb"));

        Definitions.Add("ICONS_EXPORTS");

        PlatformSettings.Windows ??= new WindowsSettings();
    }
}
