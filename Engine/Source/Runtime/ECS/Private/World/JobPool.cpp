#include "ECS/JobPool.h"

#include <algorithm>
#include <thread>

namespace we::runtime::ecs {

JobPool::JobPool(std::uint32_t workerCount) {
    if (workerCount == 0) {
        const std::uint32_t hw = std::max(1u, static_cast<std::uint32_t>(std::thread::hardware_concurrency()));
        workerCount = std::max(1u, hw > 1 ? hw - 1 : 1u);
    }
    workerCount = std::min(workerCount, kMaxJobThreads);
    m_WorkerCount = workerCount;
    m_Running.store(true, std::memory_order_release);

    m_Workers.reserve(m_WorkerCount);
    for (std::uint32_t i = 0; i < m_WorkerCount; ++i) {
        m_Workers.emplace_back([this, i]() { WorkerLoop(i); });
    }
}

JobPool::~JobPool() {
    m_Running.store(false, std::memory_order_release);
    m_JobCv.notify_all();
    for (std::thread& worker : m_Workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void JobPool::ParallelFor(std::size_t count, const std::function<void(std::size_t)>& fn) {
    if (count == 0) {
        return;
    }
    if (count == 1 || m_WorkerCount == 0) {
        fn(0);
        return;
    }

    Job job{};
    job.task = fn;
    job.begin = 0;
    job.end = count;
    job.next.store(0, std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(m_JobMutex);
        m_CurrentJob = &job;
        m_ActiveJobs.store(m_WorkerCount + 1, std::memory_order_release);
    }
    m_JobCv.notify_all();

    for (std::size_t index = job.next.fetch_add(1, std::memory_order_acq_rel); index < count;
         index = job.next.fetch_add(1, std::memory_order_acq_rel)) {
        fn(index);
    }

    std::unique_lock<std::mutex> lock(m_JobMutex);
    m_WaitCv.wait(lock, [this]() { return m_ActiveJobs.load(std::memory_order_acquire) == 0; });
    m_CurrentJob = nullptr;
}

void JobPool::ParallelForRange(std::size_t begin, std::size_t end, const std::function<void(std::size_t)>& fn) {
    if (end <= begin) {
        return;
    }
    ParallelFor(end - begin, [&](std::size_t offset) { fn(begin + offset); });
}

void JobPool::WorkerLoop(std::uint32_t /*workerIndex*/) {
    while (m_Running.load(std::memory_order_acquire)) {
        Job* job = nullptr;
        {
            std::unique_lock<std::mutex> lock(m_JobMutex);
            m_JobCv.wait(lock, [this]() {
                return !m_Running.load(std::memory_order_acquire) || m_CurrentJob != nullptr;
            });
            job = m_CurrentJob;
        }
        if (!m_Running.load(std::memory_order_acquire)) {
            break;
        }
        if (!job || !job->task) {
            continue;
        }

        for (std::size_t index = job->next.fetch_add(1, std::memory_order_acq_rel); index < job->end;
             index = job->next.fetch_add(1, std::memory_order_acq_rel)) {
            job->task(index);
        }

        if (m_ActiveJobs.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            m_WaitCv.notify_all();
        }
    }
}

} // namespace we::runtime::ecs
