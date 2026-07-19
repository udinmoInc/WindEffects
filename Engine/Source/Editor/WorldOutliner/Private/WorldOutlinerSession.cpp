#include "WorldOutliner/WorldOutlinerSession.h"

namespace we::editor::outliner {
namespace {

std::shared_ptr<IWorldOutlinerRuntime> g_Runtime;

} // namespace

void WorldOutlinerSession::Install(std::shared_ptr<IWorldOutlinerRuntime> runtime) {
    g_Runtime = std::move(runtime);
}

void WorldOutlinerSession::Clear() noexcept {
    g_Runtime.reset();
}

IWorldOutlinerRuntime* WorldOutlinerSession::Runtime() noexcept {
    return g_Runtime.get();
}

IWorldOutliner* WorldOutlinerSession::Outliner() noexcept {
    return g_Runtime ? &g_Runtime->Outliner() : nullptr;
}

std::shared_ptr<IWorldOutlinerRuntime> WorldOutlinerSession::RuntimeShared() noexcept {
    return g_Runtime;
}

bool WorldOutlinerSession::IsInstalled() noexcept {
    return static_cast<bool>(g_Runtime);
}

} // namespace we::editor::outliner
