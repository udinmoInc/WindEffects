// ==============================================================================
// WindEffects — AssetTools — AssetTools.Build
// Source file for the AssetTools module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class AssetTools : ModuleRules
{
    public AssetTools(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("AssetImporter");
        PublicDependencies.Add("AssetProcessors");
        PublicDependencies.Add("AssetPipeline");
        PublicDependencies.Add("AssetCooker");
        PublicDependencies.Add("Icons");

        Definitions.Add("ASSETTOOLS_EXPORTS");
    }
}
