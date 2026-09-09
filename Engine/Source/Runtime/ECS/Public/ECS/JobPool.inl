// ==============================================================================
// WindEffects — ECS — JobPool
// Public API surface for the ECS module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
