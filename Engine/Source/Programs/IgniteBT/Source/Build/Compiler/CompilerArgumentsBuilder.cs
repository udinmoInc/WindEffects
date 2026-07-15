using IgniteBT.Build.Compiler;
using IgniteBT.Build.Toolchain;

namespace IgniteBT.Build.Compiler;

/// <summary>
/// Builds compiler command-line arguments for all supported toolchains.
/// </summary>
public static class CompilerArgumentsBuilder
{
    public static string Build(CompilerType type, CompilerOptions options)
    {
        ICompiler compiler = type switch
        {
            CompilerType.GCC => new GCCCompiler(),
            CompilerType.Clang => new ClangCompiler(),
            _ => new MSVCCompiler()
        };
        return compiler switch
        {
            MSVCCompiler msvc => msvc.BuildArguments(options),
            ClangCompiler clang => clang.BuildArguments(options),
            GCCCompiler gcc => gcc.BuildArguments(options),
            _ => string.Empty
        };
    }
}
