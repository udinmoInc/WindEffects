// ==============================================================================
// WindEffects — ContentBrowser — ContentBrowser.Build
// Source file for the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class ContentBrowser : ModuleRules
{
    public ContentBrowser(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        // Presentation/management layer — never owns asset lifetime or undo history.
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("EditorShell");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("AssetTools");
        PublicDependencies.Add("AssetImporter");
        PublicDependencies.Add("AssetPipeline");
        PublicDependencies.Add("AssetRuntime");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");
        // Undo is NOT a module dependency — Editor injects transaction callbacks to avoid
        // ContentBrowser ↔ PropertyEditor ↔ Undo cycles.

        PrivateDependencies.Add("RHI");
        PrivateDependencies.Add("Renderer");
        PrivateDependencies.Add("Menus");

        Definitions.Add("CONTENTBROWSER_EXPORTS");
    }
}
