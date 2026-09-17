// ==============================================================================
// WindEffects — IgniteBT — AdaptiveScheduler
// Source file for the IgniteBT module.
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using Serilog;

namespace IgniteBT.Build.Scheduler;

public enum IterationExecutionMode
{
    SingleTuFastPath,
    SmallBatch,
    NormalParallel,
    FullParallel
}

public sealed class ExecutionPlan
{
    public IterationExecutionMode ExecutionMode { get; init; }
    public int TargetWorkerCount { get; init; }
    public bool BypassQueueOverhead { get; init; }
    public string Rationale { get; init; } = string.Empty;
}

public static class AdaptiveScheduler
{
    public static ExecutionPlan DetermineExecutionPlan(int changedTuCount, int requestedJobs)
    {
        int availableCores = Environment.ProcessorCount;
        int maxWorkers = Math.Min(requestedJobs > 0 ? requestedJobs : availableCores, availableCores);

        if (changedTuCount <= 1)
        {
            return new ExecutionPlan
            {
                ExecutionMode = IterationExecutionMode.SingleTuFastPath,
                TargetWorkerCount = 1,
                BypassQueueOverhead = true,
                Rationale = "Single-TU fast path: Direct execution to eliminate scheduling & IPC overhead."
            };
        }

        if (changedTuCount == 2)
        {
            return new ExecutionPlan
            {
                ExecutionMode = IterationExecutionMode.SmallBatch,
                TargetWorkerCount = 2,
                BypassQueueOverhead = false,
                Rationale = "Small batch mode: 2 parallel workers for minimal scheduling contention."
            };
        }

        if (changedTuCount <= 12)
        {
            int workers = Math.Clamp(changedTuCount, 2, maxWorkers);
            return new ExecutionPlan
            {
                ExecutionMode = IterationExecutionMode.NormalParallel,
                TargetWorkerCount = workers,
                BypassQueueOverhead = false,
                Rationale = $"Normal parallel mode: Scaled to {workers} workers based on {changedTuCount} active TUs."
            };
        }

        return new ExecutionPlan
        {
            ExecutionMode = IterationExecutionMode.FullParallel,
            TargetWorkerCount = maxWorkers,
            BypassQueueOverhead = false,
            Rationale = $"Full parallel mode: Max throughput across all {maxWorkers} parallel workers."
        };
    }
}
