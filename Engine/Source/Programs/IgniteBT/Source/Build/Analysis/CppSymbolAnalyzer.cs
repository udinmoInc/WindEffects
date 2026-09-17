// ==============================================================================
// WindEffects — IgniteBT — CppSymbolAnalyzer
// Source file for the IgniteBT module.
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using System.Text.RegularExpressions;

namespace IgniteBT.Build.Analysis;

public enum CppModificationCategory
{
    FunctionImplementation,
    ClassImplementation,
    ClassDeclaration,
    Template,
    Macro,
    GlobalVariable,
    Constexpr,
    InlineFunction,
    PublicApi,
    PrivateImplementation
}

public sealed class CppSymbolAnalysisResult
{
    public CppModificationCategory PrimaryCategory { get; init; }
    public bool IsPrivateScope { get; init; }
    public bool RequiresHeaderDependentInvalidation { get; init; }
    public string Summary { get; init; } = string.Empty;
}

public static class CppSymbolAnalyzer
{
    private static readonly Regex FunctionBodyRegex = new(@"\b[A-Za-z0-9_]+::[A-Za-z0-9_]+\s*\(.*?\)\s*\{", RegexOptions.Compiled);
    private static readonly Regex MacroRegex = new(@"^\s*#\s*define\b", RegexOptions.Compiled | RegexOptions.Multiline);
    private static readonly Regex TemplateRegex = new(@"\btemplate\s*<.*?>", RegexOptions.Compiled);
    private static readonly Regex ClassDeclRegex = new(@"\b(class|struct|union|enum)\s+[A-Za-z0-9_]+", RegexOptions.Compiled);
    private static readonly Regex ConstexprRegex = new(@"\bconstexpr\b", RegexOptions.Compiled);
    private static readonly Regex InlineRegex = new(@"\binline\b", RegexOptions.Compiled);

    public static CppSymbolAnalysisResult AnalyzeChange(string filePath, string oldContent, string newContent)
    {
        var ext = Path.GetExtension(filePath);
        bool isCpp = ext.Equals(".cpp", StringComparison.OrdinalIgnoreCase) ||
                     ext.Equals(".cxx", StringComparison.OrdinalIgnoreCase) ||
                     ext.Equals(".cc", StringComparison.OrdinalIgnoreCase);

        if (isCpp)
        {
            // If the edit is inside a .cpp file and does not modify class/struct declarations or macros
            bool hasMacroChange = MacroRegex.IsMatch(newContent) && !MacroRegex.IsMatch(oldContent);
            bool hasClassDeclChange = ClassDeclRegex.IsMatch(newContent) && !ClassDeclRegex.IsMatch(oldContent);

            if (!hasMacroChange && !hasClassDeclChange)
            {
                return new CppSymbolAnalysisResult
                {
                    PrimaryCategory = CppModificationCategory.PrivateImplementation,
                    IsPrivateScope = true,
                    RequiresHeaderDependentInvalidation = false,
                    Summary = "Private C++ implementation change inside containing TU only."
                };
            }
        }

        // Header / Structural change analysis
        if (MacroRegex.IsMatch(newContent))
        {
            return new CppSymbolAnalysisResult
            {
                PrimaryCategory = CppModificationCategory.Macro,
                IsPrivateScope = false,
                RequiresHeaderDependentInvalidation = true,
                Summary = "Preprocessor macro change."
            };
        }

        if (TemplateRegex.IsMatch(newContent))
        {
            return new CppSymbolAnalysisResult
            {
                PrimaryCategory = CppModificationCategory.Template,
                IsPrivateScope = false,
                RequiresHeaderDependentInvalidation = true,
                Summary = "Template definition change."
            };
        }

        if (ClassDeclRegex.IsMatch(newContent))
        {
            return new CppSymbolAnalysisResult
            {
                PrimaryCategory = CppModificationCategory.ClassDeclaration,
                IsPrivateScope = false,
                RequiresHeaderDependentInvalidation = true,
                Summary = "Class/struct declaration change."
            };
        }

        return new CppSymbolAnalysisResult
        {
            PrimaryCategory = CppModificationCategory.FunctionImplementation,
            IsPrivateScope = isCpp,
            RequiresHeaderDependentInvalidation = !isCpp,
            Summary = isCpp ? "Function implementation change." : "Header function change."
        };
    }
}
