#pragma once

namespace we::runtime::ecs {

template <typename Fn>
void JobPool::ParallelForChunks(const std::vector<Chunk*>& chunks, Fn&& fn) {
    ParallelFor(chunks.size(), [&](std::size_t index) {
        if (chunks[index]) {
            fn(chunks[index]);
        }
    });
}

} // namespace we::runtime::ecs
