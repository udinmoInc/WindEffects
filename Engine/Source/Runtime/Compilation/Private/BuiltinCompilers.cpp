#include "CompilationInternal.h"

#include <sstream>

namespace we::runtime::compilation {
namespace detail {
namespace {

class DeterministicCompilerBase : public ICompiler {
public:
    DeterministicCompilerBase(
        std::string name,
        CompilerKind kind,
        std::uint32_t version,
        std::vector<CompileTargetFormat> formats)
        : m_Name(std::move(name))
        , m_Kind(kind)
        , m_Version(version)
        , m_Formats(std::move(formats))
    {}

    [[nodiscard]] std::string_view GetName() const noexcept override { return m_Name; }
    [[nodiscard]] CompilerKind GetKind() const noexcept override { return m_Kind; }
    [[nodiscard]] std::uint32_t GetVersion() const noexcept override { return m_Version; }

    [[nodiscard]] bool Supports(CompilerKind kind, CompileTargetFormat format) const noexcept override {
        if (kind != m_Kind) {
            return false;
        }
        if (format == CompileTargetFormat::Auto) {
            return true;
        }
        for (auto f : m_Formats) {
            if (f == format) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] std::vector<CompileTargetFormat> SupportedFormats() const override { return m_Formats; }

    [[nodiscard]] std::string MakeCacheKey(const ICompileRequest& request) const override {
        return MakeDeterministicKey(m_Kind, m_Version, request.GetSource(), request.GetOptions());
    }

    [[nodiscard]] std::shared_ptr<ICompileResult> Compile(
        const ICompileRequest& request,
        const ICompileTask& task) override
    {
        auto result = std::make_shared<ResultImpl>();
        result->requestId = request.GetId();
        if (task.IsCancelled()) {
            result->status = CompileStatus::Cancelled;
            return result;
        }

        // Centralized deterministic artifact: hash(source + options + kind).
        // Real DXC/Metal backends plug in here without changing the pipeline.
        const auto key = MakeCacheKey(request);
        auto art = std::make_shared<ArtifactImpl>();
        art->id = CompileId{HashString(key)};
        art->kind = m_Kind;
        art->format = request.GetOptions().target == CompileTargetFormat::Auto
            ? (m_Formats.empty() ? CompileTargetFormat::BinaryBlob : m_Formats.front())
            : request.GetOptions().target;
        art->key = key;
        art->compilerVersion = m_Version;

        std::ostringstream payload;
        payload << "WEARTIFACT|" << m_Name << '|' << key << '|' << request.GetSource().path << '|'
                << request.GetOptions().entryPoint << '|' << request.GetOptions().stage;
        const auto text = payload.str();
        art->bytes.assign(text.begin(), text.end());
        if (!request.GetSource().inlineBytes.empty()) {
            art->bytes.insert(
                art->bytes.end(),
                request.GetSource().inlineBytes.begin(),
                request.GetSource().inlineBytes.end());
        }
        art->contentHash = HashBytes(art->bytes);

        if (request.GetSource().path.find("FAIL") != std::string::npos) {
            result->status = CompileStatus::Failed;
            CompileDiagnostic d;
            d.severity = CompileSeverity::Error;
            d.code = "COMP_SRC";
            d.message = "Synthetic failure for path containing FAIL";
            d.file = request.GetSource().path;
            result->diagnostics.push_back(d);
            return result;
        }

        result->status = CompileStatus::Succeeded;
        result->artifact = std::move(art);
        return result;
    }

protected:
    std::string m_Name;
    CompilerKind m_Kind = CompilerKind::Unknown;
    std::uint32_t m_Version = 1;
    std::vector<CompileTargetFormat> m_Formats;
};

} // namespace

void RegisterBuiltinCompilers(ICompilerRegistry& registry, const CompilationDependencies& deps) {
    (void)deps;
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "ShaderHLSL",
        CompilerKind::ShaderHLSL,
        1,
        std::vector<CompileTargetFormat>{CompileTargetFormat::SpirV, CompileTargetFormat::Dxil}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "ShaderGLSL", CompilerKind::ShaderGLSL, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::Glsl}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "SpirV", CompilerKind::SpirV, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::SpirV}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "Dxil", CompilerKind::Dxil, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::Dxil}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "Metal", CompilerKind::Metal, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::Metallib}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "Vulkan", CompilerKind::Vulkan, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::SpirV}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "DirectX", CompilerKind::DirectX, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::Dxil}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "OpenGL", CompilerKind::OpenGL, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::Glsl}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "Compute", CompilerKind::Compute, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::SpirV, CompileTargetFormat::Dxil}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "Material", CompilerKind::Material, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::ReflectionMeta}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "MaterialGraph", CompilerKind::MaterialGraph, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::ReflectionMeta}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "AnimationGraph", CompilerKind::AnimationGraph, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::BinaryBlob}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "VisualScriptGraph", CompilerKind::VisualScriptGraph, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::BinaryBlob}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "UiShader", CompilerKind::UiShader, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::SpirV, CompileTargetFormat::Dxil}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "TerrainShader", CompilerKind::TerrainShader, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::SpirV}));
    (void)registry.Register(std::make_shared<DeterministicCompilerBase>(
        "MeshProcess", CompilerKind::MeshProcess, 1, std::vector<CompileTargetFormat>{CompileTargetFormat::BinaryBlob}));
}

} // namespace detail
} // namespace we::runtime::compilation
