#include "CompilationInternal.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace we::runtime::compilation {
namespace detail {

std::uint64_t HashBytes(std::span<const std::uint8_t> bytes) noexcept {
    std::uint64_t h = 14695981039346656037ull;
    for (std::uint8_t b : bytes) {
        h ^= b;
        h *= 1099511628211ull;
    }
    return h;
}

std::uint64_t HashString(std::string_view text) noexcept {
    return HashBytes(std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(text.data()), text.size()));
}

std::string MakeDeterministicKey(
    CompilerKind kind,
    std::uint32_t compilerVersion,
    const CompileSourceDesc& source,
    const CompileOptions& options)
{
    std::ostringstream oss;
    oss << static_cast<int>(kind) << '|' << compilerVersion << '|' << source.path << '|'
        << source.virtualPath << '|' << source.contentHash << '|'
        << static_cast<int>(options.target) << '|' << options.entryPoint << '|' << options.stage
        << '|' << options.optimizationLevel << '|';
    for (const auto& d : options.defines) {
        oss << d << ';';
    }
    for (const auto& i : options.includePaths) {
        oss << i << ';';
    }
    if (!source.inlineBytes.empty()) {
        oss << HashBytes(source.inlineBytes);
    }
    const auto text = oss.str();
    std::ostringstream key;
    key << "wecomp_" << std::hex << HashString(text);
    return key.str();
}

class ProgressImpl final : public ICompileProgress {
public:
    std::atomic<std::uint64_t> queued{0};
    std::atomic<std::uint64_t> running{0};
    std::atomic<std::uint64_t> completed{0};
    std::atomic<std::uint64_t> failed{0};
    std::atomic<std::uint64_t> cacheHits{0};

    [[nodiscard]] std::uint64_t Queued() const noexcept override { return queued.load(); }
    [[nodiscard]] std::uint64_t Running() const noexcept override { return running.load(); }
    [[nodiscard]] std::uint64_t Completed() const noexcept override { return completed.load(); }
    [[nodiscard]] std::uint64_t Failed() const noexcept override { return failed.load(); }
    [[nodiscard]] std::uint64_t CacheHits() const noexcept override { return cacheHits.load(); }
    [[nodiscard]] float OverallFraction() const noexcept override {
        const auto done = completed.load() + failed.load();
        const auto total = done + queued.load() + running.load();
        if (total == 0) {
            return 1.f;
        }
        return static_cast<float>(done) / static_cast<float>(total);
    }
};

class NotificationImpl final : public ICompileNotification {
public:
    void Subscribe(CompileNotificationFn listener) override {
        std::lock_guard lock(m_Mutex);
        m_Listeners.push_back(std::move(listener));
    }

    void Publish(const ICompileResult& result) override {
        std::vector<CompileNotificationFn> copy;
        {
            std::lock_guard lock(m_Mutex);
            copy = m_Listeners;
        }
        for (auto& fn : copy) {
            if (fn) {
                fn(result);
            }
        }
    }

private:
    std::mutex m_Mutex;
    std::vector<CompileNotificationFn> m_Listeners;
};

class DiagnosticsSinkImpl final : public ICompileDiagnostics {
public:
    void Report(const CompileDiagnostic& diagnostic) override {
        std::lock_guard lock(m_Mutex);
        m_Items.push_back(diagnostic);
    }

    std::vector<CompileDiagnostic> Snapshot() const override {
        std::lock_guard lock(m_Mutex);
        return m_Items;
    }

    void Clear() override {
        std::lock_guard lock(m_Mutex);
        m_Items.clear();
    }

private:
    mutable std::mutex m_Mutex;
    std::vector<CompileDiagnostic> m_Items;
};

class RegistryImpl final : public ICompilerRegistry {
public:
    bool Register(std::shared_ptr<ICompiler> compiler) override {
        if (!compiler) {
            return false;
        }
        std::lock_guard lock(m_Mutex);
        m_ByKind[compiler->GetKind()] = compiler;
        m_ByName[std::string(compiler->GetName())] = compiler;
        return true;
    }

    bool Unregister(CompilerKind kind) override {
        std::lock_guard lock(m_Mutex);
        auto it = m_ByKind.find(kind);
        if (it == m_ByKind.end()) {
            return false;
        }
        m_ByName.erase(std::string(it->second->GetName()));
        m_ByKind.erase(it);
        return true;
    }

    std::shared_ptr<ICompiler> Find(CompilerKind kind) const override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_ByKind.find(kind);
        return it == m_ByKind.end() ? nullptr : it->second;
    }

    std::shared_ptr<ICompiler> FindByName(std::string_view name) const override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_ByName.find(std::string(name));
        return it == m_ByName.end() ? nullptr : it->second;
    }

    std::vector<CompilerKind> ListKinds() const override {
        std::lock_guard lock(m_Mutex);
        std::vector<CompilerKind> out;
        out.reserve(m_ByKind.size());
        for (const auto& [k, _] : m_ByKind) {
            out.push_back(k);
        }
        return out;
    }

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<CompilerKind, std::shared_ptr<ICompiler>> m_ByKind;
    std::unordered_map<std::string, std::shared_ptr<ICompiler>> m_ByName;
};

class CacheImpl final : public ICompileCache {
public:
    std::shared_ptr<ICompileArtifact> TryGet(std::string_view key) const override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Entries.find(std::string(key));
        return it == m_Entries.end() ? nullptr : it->second;
    }

    bool Put(std::shared_ptr<ICompileArtifact> artifact) override {
        if (!artifact || artifact->GetKey().empty()) {
            return false;
        }
        std::lock_guard lock(m_Mutex);
        m_Entries[std::string(artifact->GetKey())] = std::move(artifact);
        return true;
    }

    bool Invalidate(std::string_view key) override {
        std::lock_guard lock(m_Mutex);
        return m_Entries.erase(std::string(key)) > 0;
    }

    bool InvalidateByDependency(std::string_view dependencyPath) override {
        std::lock_guard lock(m_Mutex);
        std::vector<std::string> doomed;
        for (const auto& [key, art] : m_Entries) {
            if (key.find(dependencyPath) != std::string::npos) {
                doomed.push_back(key);
            }
        }
        for (const auto& k : doomed) {
            m_Entries.erase(k);
        }
        return !doomed.empty();
    }

    void Clear() override {
        std::lock_guard lock(m_Mutex);
        m_Entries.clear();
    }

    std::uint64_t EntryCount() const noexcept override {
        std::lock_guard lock(m_Mutex);
        return m_Entries.size();
    }

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<ICompileArtifact>> m_Entries;
};

class DatabaseImpl final : public ICompileDatabase {
public:
    explicit DatabaseImpl(ICompileCache& cache)
        : m_Cache(cache)
    {}

    bool Record(const ICompileResult& result) override {
        auto art = result.GetArtifact();
        if (!art) {
            return false;
        }
        std::lock_guard lock(m_Mutex);
        m_Keys.insert(std::string(art->GetKey()));
        return m_Cache.Put(art);
    }

    std::shared_ptr<ICompileArtifact> FindArtifact(std::string_view key) const override {
        return m_Cache.TryGet(key);
    }

    std::vector<std::string> ListKeys() const override {
        std::lock_guard lock(m_Mutex);
        return std::vector<std::string>(m_Keys.begin(), m_Keys.end());
    }

    bool SaveToDisk(std::string_view path) override {
        std::ofstream out(std::string(path), std::ios::trunc);
        if (!out) {
            return false;
        }
        out << "WECOMPDB 1\n";
        for (const auto& key : ListKeys()) {
            auto art = m_Cache.TryGet(key);
            if (!art) {
                continue;
            }
            out << "ART|" << key << '|' << art->GetBytes().size() << '|' << art->GetContentHash()
                << '|' << static_cast<int>(art->GetKind()) << '|'
                << static_cast<int>(art->GetFormat()) << '\n';
            out << "BYTES|";
            for (std::uint8_t b : art->GetBytes()) {
                out << std::hex << static_cast<int>(b) << ' ';
            }
            out << std::dec << '\n';
        }
        return true;
    }

    bool LoadFromDisk(std::string_view path) override {
        std::ifstream input{std::string(path)};
        if (!input) {
            return false;
        }
        std::string line;
        if (!std::getline(input, line) || line.rfind("WECOMPDB", 0) != 0) {
            return false;
        }
        while (std::getline(input, line)) {
            if (line.rfind("ART|", 0) != 0) {
                continue;
            }
            // Minimal restore: keys only; full byte restore via BYTES line.
            std::string bytesLine;
            if (!std::getline(input, bytesLine) || bytesLine.rfind("BYTES|", 0) != 0) {
                continue;
            }
            auto art = std::make_shared<ArtifactImpl>();
            // Parse ART|key|size|hash|kind|format
            std::vector<std::string> parts;
            {
                std::string cur;
                for (char c : line) {
                    if (c == '|') {
                        parts.push_back(cur);
                        cur.clear();
                    } else {
                        cur.push_back(c);
                    }
                }
                parts.push_back(cur);
            }
            if (parts.size() < 6) {
                continue;
            }
            art->key = parts[1];
            art->contentHash = std::strtoull(parts[3].c_str(), nullptr, 10);
            art->kind = static_cast<CompilerKind>(std::atoi(parts[4].c_str()));
            art->format = static_cast<CompileTargetFormat>(std::atoi(parts[5].c_str()));
            art->id = CompileId{HashString(art->key)};
            std::istringstream hex(bytesLine.substr(6));
            int v = 0;
            while (hex >> std::hex >> v) {
                art->bytes.push_back(static_cast<std::uint8_t>(v));
            }
            (void)m_Cache.Put(art);
            std::lock_guard lock(m_Mutex);
            m_Keys.insert(art->key);
        }
        return true;
    }

private:
    ICompileCache& m_Cache;
    mutable std::mutex m_Mutex;
    std::unordered_set<std::string> m_Keys;
};

class DependencyGraphImpl final : public ICompileDependencyGraph {
public:
    void AddEdge(std::string_view from, std::string_view to) override {
        std::lock_guard lock(m_Mutex);
        m_Out[std::string(from)].insert(std::string(to));
        m_In[std::string(to)].insert(std::string(from));
    }

    void RemoveNode(std::string_view node) override {
        std::lock_guard lock(m_Mutex);
        const std::string n(node);
        for (const auto& d : m_Out[n]) {
            m_In[d].erase(n);
        }
        for (const auto& d : m_In[n]) {
            m_Out[d].erase(n);
        }
        m_Out.erase(n);
        m_In.erase(n);
    }

    std::vector<std::string> GetDependents(std::string_view node) const override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_In.find(std::string(node));
        if (it == m_In.end()) {
            return {};
        }
        return std::vector<std::string>(it->second.begin(), it->second.end());
    }

    std::vector<std::string> GetDependencies(std::string_view node) const override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Out.find(std::string(node));
        if (it == m_Out.end()) {
            return {};
        }
        return std::vector<std::string>(it->second.begin(), it->second.end());
    }

    bool HasCycle() const override {
        std::lock_guard lock(m_Mutex);
        std::unordered_set<std::string> visiting;
        std::unordered_set<std::string> visited;
        for (const auto& [n, _] : m_Out) {
            if (Dfs(n, visiting, visited)) {
                return true;
            }
        }
        return false;
    }

    std::vector<std::string> CollectInvalidationSet(std::string_view root) const override {
        std::lock_guard lock(m_Mutex);
        std::vector<std::string> out;
        std::unordered_set<std::string> seen;
        std::vector<std::string> stack{std::string(root)};
        while (!stack.empty()) {
            auto n = stack.back();
            stack.pop_back();
            if (!seen.insert(n).second) {
                continue;
            }
            out.push_back(n);
            const auto it = m_In.find(n);
            if (it != m_In.end()) {
                for (const auto& d : it->second) {
                    stack.push_back(d);
                }
            }
        }
        return out;
    }

private:
    bool Dfs(
        const std::string& n,
        std::unordered_set<std::string>& visiting,
        std::unordered_set<std::string>& visited) const
    {
        if (visited.contains(n)) {
            return false;
        }
        if (visiting.contains(n)) {
            return true;
        }
        visiting.insert(n);
        const auto it = m_Out.find(n);
        if (it != m_Out.end()) {
            for (const auto& d : it->second) {
                if (Dfs(d, visiting, visited)) {
                    return true;
                }
            }
        }
        visiting.erase(n);
        visited.insert(n);
        return false;
    }

    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, std::unordered_set<std::string>> m_Out;
    std::unordered_map<std::string, std::unordered_set<std::string>> m_In;
};

struct JobPriorityLess {
    bool operator()(const std::shared_ptr<JobImpl>& a, const std::shared_ptr<JobImpl>& b) const {
        // higher priority first
        if (a->priority != b->priority) {
            return static_cast<int>(a->priority) < static_cast<int>(b->priority);
        }
        return a->id.value > b->id.value;
    }
};

class TaskImpl final : public ICompileTask {
public:
    TaskImpl(
        std::shared_ptr<JobImpl> job,
        std::shared_ptr<ICompiler> compiler,
        ICompileCache& cache,
        ICompileDatabase& db,
        ICompileNotification& notify,
        ICompileDiagnostics& diag,
        ProgressImpl& progress,
        bool enableCache,
        bool countQueued = true)
        : m_Job(std::move(job))
        , m_Compiler(std::move(compiler))
        , m_Cache(cache)
        , m_Db(db)
        , m_Notify(notify)
        , m_Diag(diag)
        , m_Progress(progress)
        , m_EnableCache(enableCache)
        , m_CountQueued(countQueued)
    {}

    [[nodiscard]] CompileId GetJobId() const noexcept override { return m_Job->id; }
    [[nodiscard]] bool IsCancelled() const noexcept override { return m_Job->cancel.load(); }

    std::shared_ptr<ICompileResult> Run() override {
        const auto t0 = NowMicros();
        m_Job->status.store(CompileStatus::Running);
        m_Progress.running.fetch_add(1);
        if (m_CountQueued && m_Progress.queued.load() > 0) {
            m_Progress.queued.fetch_sub(1);
        }

        auto makeFail = [&](std::string message) {
            auto result = std::make_shared<ResultImpl>();
            result->status = CompileStatus::Failed;
            result->requestId = m_Job->requestId;
            result->jobId = m_Job->id;
            result->durationMicros = NowMicros() - t0;
            CompileDiagnostic d;
            d.severity = CompileSeverity::Error;
            d.code = "COMP_FAIL";
            d.message = std::move(message);
            result->diagnostics.push_back(d);
            m_Diag.Report(d);
            m_Progress.running.fetch_sub(1);
            m_Progress.failed.fetch_add(1);
            m_Job->status.store(CompileStatus::Failed);
            CompilationDiagnostics::Get().OnComplete(result->durationMicros, false, true);
            m_Notify.Publish(*result);
            return result;
        };

        if (IsCancelled()) {
            auto result = std::make_shared<ResultImpl>();
            result->status = CompileStatus::Cancelled;
            result->requestId = m_Job->requestId;
            result->jobId = m_Job->id;
            result->durationMicros = NowMicros() - t0;
            m_Progress.running.fetch_sub(1);
            m_Job->status.store(CompileStatus::Cancelled);
            m_Notify.Publish(*result);
            return result;
        }

        if (!m_Compiler || !m_Job->request) {
            return makeFail("Missing compiler or request");
        }

        const auto key = m_Compiler->MakeCacheKey(*m_Job->request);
        if (m_EnableCache && m_Job->request->options.enableCaching && !m_Job->request->options.forceRebuild) {
            if (auto hit = m_Cache.TryGet(key)) {
                auto result = std::make_shared<ResultImpl>();
                result->status = CompileStatus::CacheHit;
                result->requestId = m_Job->requestId;
                result->jobId = m_Job->id;
                result->artifact = hit;
                result->cacheHit = true;
                result->durationMicros = NowMicros() - t0;
                m_Progress.running.fetch_sub(1);
                m_Progress.completed.fetch_add(1);
                m_Progress.cacheHits.fetch_add(1);
                m_Job->status.store(CompileStatus::CacheHit);
                CompilationDiagnostics::Get().OnComplete(result->durationMicros, true, false);
                (void)m_Db.Record(*result);
                m_Notify.Publish(*result);
                return result;
            }
        }

        auto result = m_Compiler->Compile(*m_Job->request, *this);
        if (!result) {
            return makeFail("Compiler returned null");
        }
        auto impl = std::dynamic_pointer_cast<ResultImpl>(result);
        if (impl) {
            impl->requestId = m_Job->requestId;
            impl->jobId = m_Job->id;
            impl->durationMicros = NowMicros() - t0;
            if (impl->status == CompileStatus::Succeeded && impl->artifact) {
                (void)m_Cache.Put(impl->artifact);
                (void)m_Db.Record(*impl);
            }
            for (const auto& d : impl->diagnostics) {
                m_Diag.Report(d);
            }
        }

        m_Progress.running.fetch_sub(1);
        if (result->GetStatus() == CompileStatus::Succeeded || result->GetStatus() == CompileStatus::CacheHit) {
            m_Progress.completed.fetch_add(1);
            m_Job->status.store(result->GetStatus());
            CompilationDiagnostics::Get().OnComplete(result->GetDurationMicros(), result->WasCacheHit(), false);
        } else if (result->GetStatus() == CompileStatus::Cancelled) {
            m_Job->status.store(CompileStatus::Cancelled);
        } else {
            m_Progress.failed.fetch_add(1);
            m_Job->status.store(CompileStatus::Failed);
            CompilationDiagnostics::Get().OnComplete(result->GetDurationMicros(), false, true);
        }
        m_Notify.Publish(*result);
        return result;
    }

private:
    std::shared_ptr<JobImpl> m_Job;
    std::shared_ptr<ICompiler> m_Compiler;
    ICompileCache& m_Cache;
    ICompileDatabase& m_Db;
    ICompileNotification& m_Notify;
    ICompileDiagnostics& m_Diag;
    ProgressImpl& m_Progress;
    bool m_EnableCache = true;
    bool m_CountQueued = true;
};

class SchedulerImpl final : public ICompileScheduler {
public:
    SchedulerImpl(
        ICompilerRegistry& registry,
        ICompileCache& cache,
        ICompileDatabase& db,
        ICompileNotification& notify,
        ICompileDiagnostics& diag,
        ProgressImpl& progress,
        std::uint32_t workerCount,
        bool background)
        : m_Registry(registry)
        , m_Cache(cache)
        , m_Db(db)
        , m_Notify(notify)
        , m_Diag(diag)
        , m_Progress(progress)
        , m_Background(background)
    {
        if (workerCount == 0) {
            workerCount = std::max(1u, std::thread::hardware_concurrency());
        }
        if (m_Background) {
            m_Workers.reserve(workerCount);
            for (std::uint32_t i = 0; i < workerCount; ++i) {
                m_Workers.emplace_back([this]() { WorkerLoop(); });
            }
        }
    }

    ~SchedulerImpl() override { Shutdown(); }

    void Enqueue(std::shared_ptr<ICompileJob> job) override {
        auto impl = std::dynamic_pointer_cast<JobImpl>(job);
        if (!impl) {
            return;
        }
        {
            std::lock_guard lock(m_Mutex);
            m_Jobs[impl->id] = impl;
            m_Queue.push(impl);
            m_Progress.queued.fetch_add(1);
            CompilationDiagnostics::Get().OnQueueDepth(m_Queue.size());
        }
        m_Cv.notify_one();
        if (!m_Background) {
            Tick();
        }
    }

    bool Cancel(CompileId jobId) override {
        std::lock_guard lock(m_Mutex);
        auto it = m_Jobs.find(jobId);
        if (it == m_Jobs.end()) {
            return false;
        }
        it->second->Cancel();
        return true;
    }

    bool Restart(CompileId jobId) override {
        std::shared_ptr<JobImpl> existing;
        {
            std::lock_guard lock(m_Mutex);
            auto it = m_Jobs.find(jobId);
            if (it == m_Jobs.end() || !it->second->request) {
                return false;
            }
            existing = it->second;
        }
        auto neu = std::make_shared<JobImpl>();
        neu->id = CompileId{++m_NextJob};
        neu->requestId = existing->requestId;
        neu->priority = existing->priority;
        neu->request = existing->request;
        neu->status.store(CompileStatus::Queued);
        Enqueue(neu);
        return true;
    }

    void Tick() override {
        if (m_Background) {
            return;
        }
        std::shared_ptr<JobImpl> job;
        {
            std::lock_guard lock(m_Mutex);
            if (m_Queue.empty()) {
                return;
            }
            job = m_Queue.top();
            m_Queue.pop();
        }
        if (job) {
            RunJob(job);
        }
    }

    void Shutdown() override {
        {
            std::lock_guard lock(m_Mutex);
            m_Stop = true;
        }
        m_Cv.notify_all();
        for (auto& t : m_Workers) {
            if (t.joinable()) {
                t.join();
            }
        }
        m_Workers.clear();
    }

    ICompileProgress& Progress() noexcept override { return m_Progress; }

    void WaitIdle() {
        for (;;) {
            {
                std::lock_guard lock(m_Mutex);
                if (m_Queue.empty() && m_Active == 0) {
                    return;
                }
            }
            if (!m_Background) {
                Tick();
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    std::shared_ptr<JobImpl> FindJob(CompileId id) {
        std::lock_guard lock(m_Mutex);
        auto it = m_Jobs.find(id);
        return it == m_Jobs.end() ? nullptr : it->second;
    }

    std::atomic<std::uint64_t> m_NextJob{0};

private:
    void WorkerLoop() {
        for (;;) {
            std::shared_ptr<JobImpl> job;
            {
                std::unique_lock lock(m_Mutex);
                m_Cv.wait(lock, [this]() { return m_Stop || !m_Queue.empty(); });
                if (m_Stop && m_Queue.empty()) {
                    return;
                }
                job = m_Queue.top();
                m_Queue.pop();
                ++m_Active;
            }
            RunJob(job);
            {
                std::lock_guard lock(m_Mutex);
                --m_Active;
            }
        }
    }

    void RunJob(const std::shared_ptr<JobImpl>& job) {
        if (!job || job->cancel.load()) {
            return;
        }
        auto compiler = m_Registry.Find(job->request ? job->request->kind : CompilerKind::Unknown);
        auto task = std::make_shared<TaskImpl>(
            job, compiler, m_Cache, m_Db, m_Notify, m_Diag, m_Progress, true);
        (void)task->Run();
    }

    ICompilerRegistry& m_Registry;
    ICompileCache& m_Cache;
    ICompileDatabase& m_Db;
    ICompileNotification& m_Notify;
    ICompileDiagnostics& m_Diag;
    ProgressImpl& m_Progress;
    bool m_Background = true;
    std::mutex m_Mutex;
    std::condition_variable m_Cv;
    std::priority_queue<std::shared_ptr<JobImpl>, std::vector<std::shared_ptr<JobImpl>>, JobPriorityLess> m_Queue;
    std::unordered_map<CompileId, std::shared_ptr<JobImpl>, CompileIdHash> m_Jobs;
    std::vector<std::thread> m_Workers;
    bool m_Stop = false;
    std::uint32_t m_Active = 0;
};

class PipelineImpl final : public ICompilePipeline {
public:
    explicit PipelineImpl(ICompilationManager& manager)
        : m_Manager(manager)
    {}

    std::shared_ptr<ICompileResult> ExecuteSync(std::shared_ptr<ICompileRequest> request) override {
        return m_Manager.CompileSync(std::move(request));
    }

    CompileId ExecuteAsync(std::shared_ptr<ICompileRequest> request) override {
        return m_Manager.CompileAsync(std::move(request));
    }

private:
    ICompilationManager& m_Manager;
};

class ManagerImpl final : public ICompilationManager {
public:
    explicit ManagerImpl(CompilationDependencies deps)
        : m_Deps(std::move(deps))
        , m_Progress()
        , m_Notify()
        , m_Diag()
        , m_Registry()
        , m_Cache()
        , m_Database(m_Cache)
        , m_DepsGraph()
        , m_Scheduler(
              m_Registry,
              m_Cache,
              m_Database,
              m_Notify,
              m_Diag,
              m_Progress,
              m_Deps.config.workerCount,
              m_Deps.config.enableBackgroundCompilation)
        , m_Pipeline(*this)
    {
        if (!m_Deps.config.databasePath.empty()) {
            (void)m_Database.LoadFromDisk(m_Deps.config.databasePath);
        }
    }

    ICompilerRegistry& Registry() noexcept override { return m_Registry; }
    ICompileCache& Cache() noexcept override { return m_Cache; }
    ICompileDatabase& Database() noexcept override { return m_Database; }
    ICompileDependencyGraph& Dependencies() noexcept override { return m_DepsGraph; }
    ICompileScheduler& Scheduler() noexcept override { return m_Scheduler; }
    ICompilePipeline& Pipeline() noexcept override { return m_Pipeline; }
    ICompileNotification& Notifications() noexcept override { return m_Notify; }
    ICompileDiagnostics& Diagnostics() noexcept override { return m_Diag; }
    ICompileProgress& Progress() noexcept override { return m_Progress; }

    std::shared_ptr<ICompileRequest> CreateRequest(
        CompilerKind kind,
        CompileSourceDesc source,
        CompileOptions options,
        CompilePriority priority) override
    {
        auto req = std::make_shared<RequestImpl>();
        req->id = CompileId{++m_NextRequest};
        req->kind = kind;
        req->source = std::move(source);
        if (req->source.contentHash == 0) {
            if (!req->source.inlineBytes.empty()) {
                req->source.contentHash = HashBytes(req->source.inlineBytes);
            } else {
                req->source.contentHash = HashString(req->source.path);
            }
        }
        req->options = std::move(options);
        req->priority = priority;
        if (!req->source.path.empty()) {
            req->dependencies.push_back(req->source.path);
        }
        return req;
    }

    std::shared_ptr<ICompileResult> CompileSync(std::shared_ptr<ICompileRequest> request) override {
        CompilationDiagnostics::Get().OnSubmit();
        auto req = std::dynamic_pointer_cast<RequestImpl>(request);
        if (!req) {
            auto fail = std::make_shared<ResultImpl>();
            fail->status = CompileStatus::Failed;
            return fail;
        }
        auto job = std::make_shared<JobImpl>();
        job->id = CompileId{++m_Scheduler.m_NextJob};
        job->requestId = req->id;
        job->priority = req->priority;
        job->request = req;
        job->status.store(CompileStatus::Scheduled);

        auto compiler = m_Registry.Find(req->kind);
        auto task = std::make_shared<TaskImpl>(
            job,
            compiler,
            m_Cache,
            m_Database,
            m_Notify,
            m_Diag,
            m_Progress,
            m_Deps.config.enableIncremental,
            false /* countQueued */);
        // Sync path: don't double-count queued
        return task->Run();
    }

    CompileId CompileAsync(std::shared_ptr<ICompileRequest> request) override {
        CompilationDiagnostics::Get().OnSubmit();
        auto req = std::dynamic_pointer_cast<RequestImpl>(request);
        if (!req) {
            return {};
        }
        auto job = std::make_shared<JobImpl>();
        job->id = CompileId{++m_Scheduler.m_NextJob};
        job->requestId = req->id;
        job->priority = req->priority;
        job->request = req;
        job->status.store(CompileStatus::Queued);
        m_Scheduler.Enqueue(job);
        return job->id;
    }

    bool Cancel(CompileId jobId) override { return m_Scheduler.Cancel(jobId); }
    bool Restart(CompileId jobId) override { return m_Scheduler.Restart(jobId); }

    std::vector<CompileId> CompileBatch(
        std::vector<std::shared_ptr<ICompileRequest>> requests,
        bool wait) override
    {
        std::vector<CompileId> ids;
        ids.reserve(requests.size());
        for (auto& r : requests) {
            ids.push_back(CompileAsync(std::move(r)));
        }
        if (wait) {
            WaitForIdle();
        }
        return ids;
    }

    std::vector<CompileId> InvalidateAndRebuild(std::string_view sourcePath) override {
        CompilationDiagnostics::Get().OnInvalidate();
        (void)m_Cache.InvalidateByDependency(sourcePath);
        const auto set = m_DepsGraph.CollectInvalidationSet(sourcePath);
        std::vector<CompileId> ids;
        for (const auto& node : set) {
            CompileSourceDesc src;
            src.path = node;
            auto req = CreateRequest(CompilerKind::ShaderHLSL, src, {}, CompilePriority::High);
            auto* r = static_cast<RequestImpl*>(req.get());
            r->options.forceRebuild = true;
            ids.push_back(CompileAsync(req));
        }
        return ids;
    }

    void WaitForIdle() override { m_Scheduler.WaitIdle(); }

    void Tick(float) override { m_Scheduler.Tick(); }

private:
    CompilationDependencies m_Deps;
    ProgressImpl m_Progress;
    NotificationImpl m_Notify;
    DiagnosticsSinkImpl m_Diag;
    RegistryImpl m_Registry;
    CacheImpl m_Cache;
    DatabaseImpl m_Database;
    DependencyGraphImpl m_DepsGraph;
    SchedulerImpl m_Scheduler;
    PipelineImpl m_Pipeline;
    std::atomic<std::uint64_t> m_NextRequest{0};
};

class CompilationRuntimeImpl final : public ICompilationRuntime {
public:
    explicit CompilationRuntimeImpl(CompilationDependencies deps)
        : m_Deps(std::move(deps))
        , m_Manager(m_Deps)
    {
        if (m_Deps.onLog) {
            m_Deps.onLog("CompilationRuntime initialized");
        }
    }

    ICompilationManager& Manager() noexcept override { return m_Manager; }
    const ICompilationManager& Manager() const noexcept override { return m_Manager; }

    void RegisterBuiltinCompilers() override {
        detail::RegisterBuiltinCompilers(m_Manager.Registry(), m_Deps);
    }

    void Shutdown() override {
        m_Manager.Scheduler().Shutdown();
        if (!m_Deps.config.databasePath.empty()) {
            (void)m_Manager.Database().SaveToDisk(m_Deps.config.databasePath);
        }
        if (m_Deps.onLog) {
            m_Deps.onLog("CompilationRuntime shutdown");
        }
    }

private:
    CompilationDependencies m_Deps;
    ManagerImpl m_Manager;
};

} // namespace detail

std::unique_ptr<ICompilationRuntime> CreateCompilationRuntime(const CompilationDependencies& deps) {
    return std::make_unique<detail::CompilationRuntimeImpl>(deps);
}

} // namespace we::runtime::compilation
