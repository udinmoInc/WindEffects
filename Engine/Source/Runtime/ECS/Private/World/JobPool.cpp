#include "ECS/JobPool.h"

#include <algorithm>
#include <thread>

namespace we::runtime::ecs {

JobPool::JobPool(std::uint32_t workerCount) {
    if (workerCount == 0) {
        const std::uint32_t hw = std::max(1u, static_cast<std::uint32_t>(std::thread::hardware_concurrency()));
        workerCount = std::max(1u, hw > 1 ? hw - 1 : 1u);
    }
    m_WorkerCount = std::min(workerCount, kMaxJobThreads);
}

JobPool::~JobPool() {
    ShutdownWorkers();
}

void JobPool::EnsureWorkers() {
    if (m_Running.load(std::memory_order_acquire)) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_JobMutex);
    if (m_Running.load(std::memory_order_relaxed)) {
        return;
    }
    m_Running.store(true, std::memory_order_release);
    m_Workers.reserve(m_WorkerCount);
    for (std::uint32_t i = 0; i < m_WorkerCount; ++i) {
        m_Workers.emplace_back([this, i]() { WorkerLoop(i); });
    }
}

void JobPool::ShutdownWorkers() {
    std::vector<std::thread> workers;
    {
        std::lock_guard<std::mutex> lock(m_JobMutex);
        if (!m_Running.load(std::memory_order_relaxed) && m_Workers.empty()) {
            return;
        }
        m_Running.store(false, std::memory_order_release);
        m_CurrentJob = nullptr;
        ++m_JobEpoch;
        workers.swap(m_Workers);
    }
    m_JobCv.notify_all();
    for (std::thread& worker : workers) {
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
        for (std::size_t i = 0; i < count; ++i) {
            fn(i);
        }
        return;
    }

    EnsureWorkers();

    Job job{};
    job.task = fn;
    job.begin = 0;
    job.end = count;
    job.next.store(0, std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(m_JobMutex);
        m_CurrentJob = &job;
        ++m_JobEpoch;
        m_ActiveJobs.store(m_WorkerCount + 1, std::memory_order_release);
    }
    m_JobCv.notify_all();

    for (std::size_t index = job.next.fetch_add(1, std::memory_order_acq_rel); index < count;
         index = job.next.fetch_add(1, std::memory_order_acq_rel)) {
        fn(index);
    }

    if (m_ActiveJobs.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        m_WaitCv.notify_all();
    }

    {
        std::unique_lock<std::mutex> lock(m_JobMutex);
        m_WaitCv.wait(lock, [this]() {
            return m_ActiveJobs.load(std::memory_order_acquire) == 0;
        });
        m_CurrentJob = nullptr;
    }
}

void JobPool::ParallelForRange(std::size_t begin, std::size_t end, const std::function<void(std::size_t)>& fn) {
    if (end <= begin) {
        return;
    }
    ParallelFor(end - begin, [&](std::size_t offset) { fn(begin + offset); });
}

void JobPool::WorkerLoop(std::uint32_t /*workerIndex*/) {
    std::uint64_t seenEpoch = 0;
    while (true) {
        Job* job = nullptr;
        {
            std::unique_lock<std::mutex> lock(m_JobMutex);
            m_JobCv.wait(lock, [this, &seenEpoch]() {
                return !m_Running.load(std::memory_order_acquire)
                    || (m_CurrentJob != nullptr && m_JobEpoch != seenEpoch);
            });
            if (!m_Running.load(std::memory_order_acquire)) {
                break;
            }
            seenEpoch = m_JobEpoch;
            job = m_CurrentJob;
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
