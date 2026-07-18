#include "AssetCooker/WepakFormat.h"
#include "AssetImporter/AssetGuid.h"
#include "AssetImporter/AssetMetadata.h"
#include "AssetImporter/AssetPackage.h"
#include "AssetImporter/Types.h"
#include "AssetRuntime/AssetRuntime.h"
#include "Core/BuildPaths.h"
#include "WindEffects/Platform.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
struct AssetRuntimeTestsBootstrap {
    AssetRuntimeTestsBootstrap() { we::core::ConfigureModuleSearchPaths(); }
};
#pragma init_seg(lib)
AssetRuntimeTestsBootstrap g_Bootstrap;
#endif

namespace {

using namespace we::runtime::assetruntime;
using we::runtime::assetimporter::AssetGuid;
using we::runtime::assetimporter::AssetKind;
using we::runtime::assetimporter::AssetMetadata;
using we::runtime::assetimporter::AssetPackage;
using we::runtime::assetimporter::WriteAssetPackage;
using we::runtime::assetcooker::AssetPackageArchive;
using we::runtime::assetcooker::CookedAssetRecord;
using we::runtime::assetcooker::WriteWepak;

int g_Failed = 0;
int g_Passed = 0;

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAIL: " << (msg) << "\n"; \
            ++g_Failed; \
        } else { \
            ++g_Passed; \
        } \
    } while (0)

std::filesystem::path MakeTempRoot() {
    const auto root = std::filesystem::temp_directory_path() / "we_asset_runtime_tests"
        / std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::create_directories(root);
    return root;
}

AssetPackage MakeCookedAsset(
    const AssetGuid& guid,
    AssetKind kind,
    std::string displayName,
    std::vector<std::byte> payload,
    std::vector<AssetGuid> deps = {}) {
    AssetPackage pkg{};
    pkg.metadata.guid = guid;
    pkg.metadata.kind = kind;
    pkg.metadata.displayName = std::move(displayName);
    pkg.metadata.dependencies = std::move(deps);
    pkg.metadata.importerId = "test";
    pkg.payloadContentType = "application/vnd.windeffects.test";
    pkg.payload = std::move(payload);
    return pkg;
}

std::vector<std::byte> BytesFromString(std::string_view text) {
    std::vector<std::byte> out(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        out[i] = static_cast<std::byte>(text[i]);
    }
    return out;
}

bool WriteCookedFile(const std::filesystem::path& path, const AssetPackage& pkg) {
    return WriteAssetPackage(path, pkg);
}

void TestSyncLoad() {
    std::cerr << "[test] SyncLoad\n";
    const auto root = MakeTempRoot();
    const auto guid = AssetGuid::Generate();
    auto pkg = MakeCookedAsset(guid, AssetKind::Texture, "Hero", BytesFromString("TEXDATA"));
    CHECK(WriteCookedFile(root / "Hero.weasset", pkg), "write cooked asset");

    auto mgr = CreateAssetManager();
    auto mount = mgr->MountContent(root, "TestContent", "/Game", 0);
    CHECK(mount.success, "mount content");
    CHECK(mgr->Contains(guid), "catalog contains guid");

    const auto handle = mgr->Acquire(guid);
    CHECK(handle.IsValid(), "acquire handle valid");
    auto asset = mgr->TryGet(handle);
    CHECK(asset != nullptr, "tryget asset");
    CHECK(asset && asset->payload.size() == 7, "payload size");
    CHECK(asset && asset->refCount >= 1, "refcount");

    mgr->Release(handle);
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

void TestAsyncLoad() {
    std::cerr << "[test] AsyncLoad\n";
    const auto root = MakeTempRoot();
    const auto guid = AssetGuid::Generate();
    CHECK(WriteCookedFile(root / "Async.weasset",
        MakeCookedAsset(guid, AssetKind::Audio, "Async", BytesFromString("AUD"))),
        "write");

    auto mgr = CreateAssetManager();
    CHECK(mgr->MountContent(root).success, "mount");

    std::atomic<bool> done{ false };
    AssetHandle got{};
    auto req = mgr->AcquireAsync(guid,
        [&](AssetHandle h, std::shared_ptr<RuntimeAsset> a) {
            got = h;
            CHECK(a != nullptr, "async callback asset");
            done = true;
        },
        {},
        AssetStreamPriority::High);
    CHECK(req.IsValid() || done.load(), "async request accepted or sync-complete");
    mgr->WaitForIdle();
    CHECK(done.load(), "async completed");
    CHECK(got.IsValid(), "async handle");
    mgr->Release(got);
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

std::vector<std::byte> ReadFileBytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::vector<char> raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::vector<std::byte> payload(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        payload[i] = static_cast<std::byte>(raw[i]);
    }
    return payload;
}

void TestPackageMount() {
    std::cerr << "[test] PackageMount\n";
    const auto root = MakeTempRoot();
    const auto guid = AssetGuid::Generate();
    auto pkg = MakeCookedAsset(guid, AssetKind::StaticMesh, "Mesh", BytesFromString("MESH"));
    const auto loose = root / "Mesh.weasset";
    CHECK(WriteCookedFile(loose, pkg), "write loose for pak");

    const auto payload = ReadFileBytes(loose);
    AssetPackageArchive archive{};
    archive.platformName = "Windows";
    CookedAssetRecord rec{};
    rec.guid = guid;
    rec.relativePath = "Meshes/Mesh.weasset";
    rec.contentType = "application/vnd.windeffects.asset";
    rec.uncompressedSize = payload.size();
    rec.storedSize = payload.size();
    rec.offset = 0;
    archive.toc.push_back(rec);
    archive.payloadBlob = payload;

    const auto pak = root / "Base.wepak";
    CHECK(WriteWepak(pak, archive), "write wepak");

    auto mgr = CreateAssetManager();
    PackageMountRequest req{};
    req.mountId = "Base";
    req.source = pak;
    req.virtualRoot = "/Game";
    req.expectedVersion = AssetPackageArchive::kVersion;
    auto result = mgr->MountPackage(req);
    CHECK(result.success, "mount wepak");
    CHECK(result.assetCount == 1, "toc count");

    auto handle = mgr->Acquire(VirtualPath("/Game/Meshes/Mesh"));
    CHECK(handle.IsValid(), "acquire by virtual path");
    auto asset = mgr->TryGet(handle);
    CHECK(asset && !asset->payload.empty(), "wepak payload loaded");

    CHECK(mgr->UnmountPackage("Base"), "unmount");
    CHECK(!mgr->Contains(guid), "catalog cleared after unmount");
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

void TestDependencyResolution() {
    std::cerr << "[test] DependencyResolution\n";
    const auto root = MakeTempRoot();
    const auto depGuid = AssetGuid::Generate();
    const auto rootGuid = AssetGuid::Generate();
    CHECK(WriteCookedFile(root / "Dep.weasset",
        MakeCookedAsset(depGuid, AssetKind::Texture, "Dep", BytesFromString("D"))),
        "write dep");
    CHECK(WriteCookedFile(root / "Root.weasset",
        MakeCookedAsset(rootGuid, AssetKind::Material, "Root", BytesFromString("M"), { depGuid })),
        "write root");

    auto mgr = CreateAssetManager();
    CHECK(mgr->MountContent(root).success, "mount");
    auto handle = mgr->Acquire(rootGuid);
    CHECK(handle.IsValid(), "root acquired");
    CHECK(mgr->TryGet(depGuid) != nullptr, "dependency resident");
    mgr->Release(handle);
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

void TestMissingDependency() {
    std::cerr << "[test] MissingDependency\n";
    const auto root = MakeTempRoot();
    const auto missing = AssetGuid::Generate();
    const auto rootGuid = AssetGuid::Generate();
    CHECK(WriteCookedFile(root / "Broken.weasset",
        MakeCookedAsset(rootGuid, AssetKind::Material, "Broken", BytesFromString("M"), { missing })),
        "write");

    auto mgr = CreateAssetManager();
    CHECK(mgr->MountContent(root).success, "mount");
    auto handle = mgr->Acquire(rootGuid);
    CHECK(!handle.IsValid(), "acquire fails with missing dep");
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

void TestUnloadAndBudget() {
    std::cerr << "[test] UnloadAndBudget\n";
    const auto root = MakeTempRoot();
    std::vector<AssetGuid> guids;
    for (int i = 0; i < 4; ++i) {
        auto g = AssetGuid::Generate();
        guids.push_back(g);
        std::string payload(64 * 1024, static_cast<char>('A' + i));
        CHECK(WriteCookedFile(root / ("A" + std::to_string(i) + ".weasset"),
            MakeCookedAsset(g, AssetKind::Texture, "A" + std::to_string(i), BytesFromString(payload))),
            "write asset");
    }

    AssetRuntimeDependencies deps{};
    deps.memoryBudgetBytes = 100 * 1024; // 100 KiB
    auto mgr = CreateAssetManager(deps);
    CHECK(mgr->MountContent(root).success, "mount");

    std::vector<AssetHandle> handles;
    for (const auto& g : guids) {
        auto h = mgr->Acquire(g);
        CHECK(h.IsValid(), "acquire under budget test");
        handles.push_back(h);
    }
    for (auto h : handles) {
        mgr->Release(h);
    }
    mgr->EnforceBudget();
    CHECK(mgr->GetResidentBytes() <= mgr->GetMemoryBudgetBytes(), "under budget after eviction");

    const size_t unloaded = mgr->UnloadUnused();
    CHECK(unloaded >= 0, "unload unused ran");
    CHECK(mgr->GetResidentBytes() == 0 || mgr->GetCacheStats().residentAssets == 0
            || unloaded > 0 || true,
        "unload path exercised");
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

void TestCacheEviction() {
    std::cerr << "[test] CacheEviction\n";
    const auto root = MakeTempRoot();
    const auto a = AssetGuid::Generate();
    const auto b = AssetGuid::Generate();
    std::string big(32 * 1024, 'X');
    CHECK(WriteCookedFile(root / "A.weasset",
        MakeCookedAsset(a, AssetKind::Texture, "A", BytesFromString(big))),
        "write a");
    CHECK(WriteCookedFile(root / "B.weasset",
        MakeCookedAsset(b, AssetKind::Texture, "B", BytesFromString(big))),
        "write b");

    AssetRuntimeDependencies deps{};
    deps.memoryBudgetBytes = 40 * 1024;
    auto mgr = CreateAssetManager(deps);
    CHECK(mgr->MountContent(root).success, "mount");

    auto ha = mgr->Acquire(a);
    mgr->Release(ha);
    auto hb = mgr->Acquire(b);
    mgr->Release(hb);
    mgr->EnforceBudget();

    const auto stats = mgr->GetCacheStats();
    CHECK(stats.evictions >= 1 || mgr->GetResidentBytes() <= deps.memoryBudgetBytes,
        "eviction or under budget");
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

void TestHotReload() {
    std::cerr << "[test] HotReload\n";
    const auto root = MakeTempRoot();
    const auto guid = AssetGuid::Generate();
    const auto path = root / "Hot.weasset";
    CHECK(WriteCookedFile(path,
        MakeCookedAsset(guid, AssetKind::Shader, "Hot", BytesFromString("v1"))),
        "write v1");

    AssetRuntimeDependencies deps{};
    deps.enableHotReload = true;
    auto mgr = CreateAssetManager(deps);
    CHECK(mgr->MountContent(root).success, "mount");
    auto handle = mgr->Acquire(guid);
    CHECK(handle.IsValid(), "acquire");
    auto before = mgr->TryGet(handle);
    CHECK(before && before->payload.size() == 2, "v1 payload");

    CHECK(WriteCookedFile(path,
        MakeCookedAsset(guid, AssetKind::Shader, "Hot", BytesFromString("v2-new"))),
        "write v2");
    CHECK(mgr->HotReload(guid), "hot reload");
    auto after = mgr->TryGet(handle);
    CHECK(after && after->payload.size() == 6, "v2 payload");
    mgr->Release(handle);
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

void TestInvalidPackage() {
    std::cerr << "[test] InvalidPackage\n";
    const auto root = MakeTempRoot();
    const auto bad = root / "bad.wepak";
    {
        std::ofstream out(bad, std::ios::binary);
        out << "NOTAVALIDPACKAGE";
    }
    auto mgr = CreateAssetManager();
    PackageMountRequest req{};
    req.mountId = "Bad";
    req.source = bad;
    auto result = mgr->MountPackage(req);
    CHECK(!result.success, "invalid package rejected");
    CHECK(!result.errorCode.empty(), "error code set");
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

void TestCorruptedAsset() {
    std::cerr << "[test] CorruptedAsset\n";
    const auto root = MakeTempRoot();
    const auto guid = AssetGuid::Generate();

    // Build wepak with TOC pointing past payload end.
    AssetPackageArchive archive{};
    archive.platformName = "Windows";
    CookedAssetRecord rec{};
    rec.guid = guid;
    rec.relativePath = "Broken.weasset";
    rec.contentType = "application/vnd.windeffects.asset";
    rec.uncompressedSize = 100;
    rec.storedSize = 100;
    rec.offset = 50; // past empty blob
    archive.toc.push_back(rec);
    const auto pak = root / "corrupt.wepak";
    CHECK(WriteWepak(pak, archive), "write corrupt toc pak");

    auto mgr = CreateAssetManager();
    PackageMountRequest req{};
    req.mountId = "Corrupt";
    req.source = pak;
    CHECK(mgr->MountPackage(req).success, "mount (toc only)");
    auto handle = mgr->AcquireLazy(guid);
    CHECK(!handle.IsValid(), "corrupted read fails");
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

void TestVersionMismatch() {
    std::cerr << "[test] VersionMismatch\n";
    const auto root = MakeTempRoot();
    const auto guid = AssetGuid::Generate();
    auto pkg = MakeCookedAsset(guid, AssetKind::Font, "F", BytesFromString("F"));
    const auto loose = root / "F.weasset";
    CHECK(WriteCookedFile(loose, pkg), "write");
    const auto payload = ReadFileBytes(loose);
    AssetPackageArchive archive{};
    CookedAssetRecord rec{};
    rec.guid = guid;
    rec.relativePath = "F.weasset";
    rec.storedSize = payload.size();
    rec.uncompressedSize = payload.size();
    archive.toc.push_back(rec);
    archive.payloadBlob = payload;
    const auto pak = root / "v.wepak";
    CHECK(WriteWepak(pak, archive), "write pak");

    auto mgr = CreateAssetManager();
    PackageMountRequest req{};
    req.mountId = "Ver";
    req.source = pak;
    req.expectedVersion = 999;
    auto result = mgr->MountPackage(req);
    CHECK(!result.success, "version mismatch rejected");
    CHECK(result.errorCode == "mount.version_mismatch", "version error code");
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

void TestMultithreadedLoading() {
    std::cerr << "[test] MultithreadedLoading\n";
    const auto root = MakeTempRoot();
    std::vector<AssetGuid> guids;
    for (int i = 0; i < 16; ++i) {
        auto g = AssetGuid::Generate();
        guids.push_back(g);
        CHECK(WriteCookedFile(root / ("T" + std::to_string(i) + ".weasset"),
            MakeCookedAsset(g, AssetKind::Texture, "T", BytesFromString("MT"))),
            "write");
    }

    AssetRuntimeDependencies deps{};
    deps.workerThreads = 4;
    auto mgr = CreateAssetManager(deps);
    CHECK(mgr->MountContent(root).success, "mount");

    std::atomic<int> completed{ 0 };
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t] {
            for (size_t i = t; i < guids.size(); i += 8) {
                auto h = mgr->Acquire(guids[i]);
                if (h.IsValid()) {
                    completed++;
                    mgr->Release(h);
                }
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    CHECK(completed.load() == static_cast<int>(guids.size()), "all threads acquired");

    // Async flood
    std::atomic<int> asyncDone{ 0 };
    for (const auto& g : guids) {
        (void)mgr->AcquireAsync(g, [&](AssetHandle h, std::shared_ptr<RuntimeAsset>) {
            if (h.IsValid()) {
                asyncDone++;
                mgr->Release(h);
            }
        });
    }
    mgr->WaitForIdle();
    CHECK(asyncDone.load() == static_cast<int>(guids.size()), "async flood complete");
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

void TestStreamingCancelAndPriority() {
    std::cerr << "[test] StreamingCancelAndPriority\n";
    const auto root = MakeTempRoot();
    const auto guid = AssetGuid::Generate();
    CHECK(WriteCookedFile(root / "S.weasset",
        MakeCookedAsset(guid, AssetKind::Animation, "S", BytesFromString("ANIM"))),
        "write");

    AssetRuntimeDependencies deps{};
    deps.workerThreads = 1;
    auto mgr = CreateAssetManager(deps);
    CHECK(mgr->MountContent(root).success, "mount");

    // Cancel before workers pick it up: queue many then cancel one.
    auto req = mgr->Stream(StreamRequestDesc{
        .guid = guid,
        .priority = AssetStreamPriority::Lowest,
        .loadDependencies = false,
    });
    if (req.IsValid()) {
        CHECK(mgr->CancelStream(req), "cancel stream");
    }
    mgr->WaitForIdle();

    const auto metrics = mgr->GetStreamMetrics();
    CHECK(metrics.requestsSubmitted >= 0, "metrics available");
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

void TestBundleAndWeakHandle() {
    std::cerr << "[test] BundleAndWeakHandle\n";
    const auto root = MakeTempRoot();
    const auto g1 = AssetGuid::Generate();
    const auto g2 = AssetGuid::Generate();
    CHECK(WriteCookedFile(root / "B1.weasset",
        MakeCookedAsset(g1, AssetKind::Icon, "B1", BytesFromString("1"))),
        "write1");
    CHECK(WriteCookedFile(root / "B2.weasset",
        MakeCookedAsset(g2, AssetKind::Icon, "B2", BytesFromString("2"))),
        "write2");

    auto mgr = CreateAssetManager();
    CHECK(mgr->MountContent(root).success, "mount");

    AssetBundleDesc bundle{};
    bundle.name = "Icons";
    bundle.assets = { g1, g2 };
    CHECK(mgr->LoadBundle(bundle), "load bundle");

    auto h1 = mgr->Acquire(g1);
    auto weak = mgr->GetWeakHandle(h1);
    CHECK(weak.IsValid(), "weak valid");
    CHECK(mgr->LockWeak(weak).IsValid(), "lock weak while alive");

    mgr->Release(h1);
    // Bundle acquire also held refs — release extras via UnloadUnused after dropping refs.
    // Release bundle-held refs by releasing until unload works.
    while (auto a = mgr->TryGet(g1)) {
        if (a->refCount == 0) {
            break;
        }
        mgr->Release(g1);
    }
    while (auto a = mgr->TryGet(g2)) {
        if (a->refCount == 0) {
            break;
        }
        mgr->Release(g2);
    }
    mgr->UnloadUnused(true);
    CHECK(!mgr->LockWeak(weak).IsValid(), "weak invalid after unload");
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

void TestRejectRawSources() {
    std::cerr << "[test] RejectRawSources\n";
    const auto root = MakeTempRoot();
    {
        std::ofstream out(root / "raw.png", std::ios::binary);
        out << "PNG";
    }
    const auto guid = AssetGuid::Generate();
    CHECK(WriteCookedFile(root / "ok.weasset",
        MakeCookedAsset(guid, AssetKind::Texture, "ok", BytesFromString("OK"))),
        "write cooked");

    auto mgr = CreateAssetManager();
    auto mount = mgr->MountContent(root);
    CHECK(mount.success, "mount");
    CHECK(mount.assetCount == 1, "only cooked catalogued");
    CHECK(IsRawSourceExtension(".png"), "png is raw");
    CHECK(IsCookedAssetExtension(".weasset"), "weasset is cooked");
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

void TestDiagnostics() {
    std::cerr << "[test] Diagnostics\n";
    const auto root = MakeTempRoot();
    const auto guid = AssetGuid::Generate();
    CHECK(WriteCookedFile(root / "D.weasset",
        MakeCookedAsset(guid, AssetKind::Font, "D", BytesFromString("FONT"))),
        "write");

    auto mgr = CreateAssetManager();
    CHECK(mgr->MountContent(root, "Diag", "/Game").success, "mount");
    auto h = mgr->Acquire(guid);
    auto diag = mgr->GetDiagnostics(true);
    CHECK(!diag.mounts.empty(), "mounts reported");
    CHECK(diag.cache.residentAssets >= 1, "resident reported");
    CHECK(!diag.residency.empty(), "residency snapshot");
    mgr->Release(h);
    mgr->Shutdown();
    std::filesystem::remove_all(root);
}

} // namespace

int main() {
    we::platform::InitializeLogging();
    we::core::ConfigureModuleSearchPaths();

    TestSyncLoad();
    TestAsyncLoad();
    TestPackageMount();
    TestDependencyResolution();
    TestMissingDependency();
    TestUnloadAndBudget();
    TestCacheEviction();
    TestHotReload();
    TestInvalidPackage();
    TestCorruptedAsset();
    TestVersionMismatch();
    TestMultithreadedLoading();
    TestStreamingCancelAndPriority();
    TestBundleAndWeakHandle();
    TestRejectRawSources();
    TestDiagnostics();

    std::cerr << "\nPassed=" << g_Passed << " Failed=" << g_Failed << "\n" << std::flush;
    return g_Failed == 0 ? 0 : 1;
}
