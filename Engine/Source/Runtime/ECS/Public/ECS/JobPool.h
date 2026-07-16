#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/Export.h"
#include "ECS/Types.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace we::runtime::ecs {

class ECS_API JobPool {
public:
    explicit JobPool(std::uint32_t workerCount = kDefaultJobThreads);
    ~JobPool();

    JobPool(const JobPool&) = delete;
    JobPool& operator=(const JobPool&) = delete;

    [[nodiscard]] std::uint32_t WorkerCount() const { return m_WorkerCount; }

    void ParallelFor(std::size_t count, const std::function<void(std::size_t)>& fn);
    void ParallelForRange(std::size_t begin, std::size_t end, const std::function<void(std::size_t)>& fn);

    template <typename Fn>
    void ParallelForChunks(const std::vector<struct Chunk*>& chunks, Fn&& fn);

private:
    struct Job {
        std::function<void(std::size_t)> task;
        std::size_t begin = 0;
        std::size_t end = 0;
        std::atomic<std::size_t> next{0};
    };

    void WorkerLoop(std::uint32_t workerIndex);

    std::uint32_t m_WorkerCount = 0;
    std::vector<std::thread> m_Workers;
    std::atomic<bool> m_Running{false};
    std::atomic<std::size_t> m_ActiveJobs{0};
    Job* m_CurrentJob = nullptr;
    std::mutex m_JobMutex;
    std::condition_variable m_JobCv;
    std::condition_variable m_WaitCv;
};

} // namespace we::runtime::ecs

#include "ECS/JobPool.inl"

#pragma warning(pop)
