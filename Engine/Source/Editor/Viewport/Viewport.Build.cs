// ==============================================================================
// WindEffects — Viewport — Viewport.Build
// Source file for the Viewport module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class Viewport : ModuleRules
{
    public Viewport(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("EditorShell");
        PrivateDependencies.Add("Toolbar");
        PrivateDependencies.Add("PlaceActors");
        PrivateDependencies.Add("Terrain");
        PrivateDependencies.Add("ViewportEdit");
        PrivateDependencies.Add("Menus");

        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");

        Definitions.Add("VIEWPORT_EXPORTS");
    }
}
