#include "Compilation/CompilationSession.h"

namespace we::runtime::compilation {
namespace {

std::shared_ptr<ICompilationRuntime> g_Runtime;

} // namespace

void CompilationSession::Install(std::shared_ptr<ICompilationRuntime> runtime) {
    g_Runtime = std::move(runtime);
}

void CompilationSession::Clear() noexcept {
    g_Runtime.reset();
}

ICompilationRuntime* CompilationSession::Runtime() noexcept {
    return g_Runtime.get();
}

ICompilationManager* CompilationSession::Manager() noexcept {
    return g_Runtime ? &g_Runtime->Manager() : nullptr;
}

std::shared_ptr<ICompilationRuntime> CompilationSession::RuntimeShared() noexcept {
    return g_Runtime;
}

bool CompilationSession::IsInstalled() noexcept {
    return static_cast<bool>(g_Runtime);
}

} // namespace we::runtime::compilation
