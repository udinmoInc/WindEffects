// ==============================================================================
// WindEffects — PlaceActors — PlaceActors.Build
// Source file for the PlaceActors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class PlaceActors : ModuleRules
{
    public PlaceActors(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("EditorShell");
        PublicDependencies.Add("ContentBrowser");
        PublicDependencies.Add("Toolbar");
        PublicDependencies.Add("Terrain");
        PrivateDependencies.Add("TerrainEditor");

        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");

        Definitions.Add("PLACEACTORS_EXPORTS");
    }
}
