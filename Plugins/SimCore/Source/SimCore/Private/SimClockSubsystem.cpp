#include "SimClockSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "HAL/PlatformTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogSimClock, Log, All);

bool USimClockSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    // Run in Game and PIE only, not in the editor viewport
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void USimClockSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    // The parent marks the subsystem as initialized; without this it never ticks
    Super::Initialize(Collection);

    UE_LOG(LogSimClock, Log, TEXT("SimClock started: FixedDt = %.4f s (%.0f Hz)"),
        FixedDt, 1.0 / FixedDt);
}

void USimClockSubsystem::Tick(float DeltaTime)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(SimClock_Tick);
    Super::Tick(DeltaTime);

    LastFrameSteps = 0;

    // Real elapsed time from the OS clock, not the engine's clamped DeltaTime
    const double Now = FPlatformTime::Seconds();
    const double RealDt = (LastWallClock > 0.0) ? (Now - LastWallClock) : 0.0;
    LastWallClock = Now;

    if (bPaused)
    {
        DrawDebug();
        return;
    }

    WallTime += RealDt;
    Accumulator += DeltaTime * TimeScale;   // 1. pour this frame's time into the bucket

    // 2. take out FixedDt chunks while there's enough
    while (Accumulator >= FixedDt && LastFrameSteps < MaxStepsPerFrame)
    {
        StepSimulation(FixedDt);
        Accumulator -= FixedDt;
        ++LastFrameSteps;
    }

    // 3. hit the step cap: drop the extra time instead of falling further behind
    if (Accumulator >= FixedDt)
    {
        const double Dropped = Accumulator - FMath::Fmod(Accumulator, FixedDt);
        TotalDropped += Dropped;   // new
        UE_LOG(LogSimClock, Verbose, TEXT("Dropped %.4f s of sim time"), Dropped);
        Accumulator = FMath::Fmod(Accumulator, FixedDt);
    }
    // close the stats window every StatsWindowSeconds
    if (StatsWindowStart <= 0.0)
    {
        StatsWindowStart = Now;   // first frame: start the first window
    }
    else if (Now - StatsWindowStart >= StatsWindowSeconds)
    {
        LastStepAvgMs = (StepTimeCount > 0) ? (StepTimeSum / StepTimeCount) * 1000.0 : 0.0;
        LastStepMaxMs = StepTimeMax * 1000.0;

        const double BudgetPct = LastStepAvgMs / (FixedDt * 1000.0) * 100.0;
        UE_LOG(LogSimClock, Log,
            TEXT("[SimStats] t=%.2fs | Step avg %.3f ms | max %.3f ms | Budget %.0f%% | Steps %d | RTF %.2f | Dropped %.2f s"),
            SimTime, LastStepAvgMs, LastStepMaxMs, BudgetPct, StepTimeCount, GetRTF(), TotalDropped);

        StepTimeSum = 0.0;
        StepTimeMax = 0.0;
        StepTimeCount = 0;
        StatsWindowStart = Now;
    }
    OnSimRender.Broadcast(GetAlpha());   // once per frame, after all steps
    DrawDebug();
}

void USimClockSubsystem::StepSimulation(double Dt)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(SimClock_Step);

    const double StepStart = FPlatformTime::Seconds();
    OnSimStep.Broadcast(Dt);                                      // the real work
    const double StepTime = FPlatformTime::Seconds() - StepStart;

    StepTimeSum += StepTime;
    StepTimeMax = FMath::Max(StepTimeMax, StepTime);
    ++StepTimeCount;

    SimTime += Dt;
    ++StepCount;
}

void USimClockSubsystem::ResetClock()
{
    Accumulator = 0.0;
    SimTime = 0.0;
    WallTime = 0.0;
    StepCount = 0;
    LastFrameSteps = 0;
    LastWallClock = 0.0;
    StatsWindowStart = 0.0;
    StepTimeSum = 0.0;
    StepTimeMax = 0.0;
    StepTimeCount = 0;
    LastStepAvgMs = 0.0;
    LastStepMaxMs = 0.0;
    TotalDropped = 0.0;
}

void USimClockSubsystem::DrawDebug() const
{
    if (!GEngine) return;

    const FColor Color = bPaused ? FColor::Yellow : FColor::Green;
    GEngine->AddOnScreenDebugMessage(
        /*Key*/ 1001, /*Time*/ 0.f, Color,
        FString::Printf(TEXT("[SimClock] %s Sim %.2fs | Wall %.2fs | RTF %.2f | Scale %.1fx | Steps %lld (+%d this frame)"),
            bPaused ? TEXT("PAUSED") : TEXT(""),
            SimTime, WallTime, GetRTF(), TimeScale, StepCount, LastFrameSteps));



    // ---- [SimStats] line ----
    const double BudgetMs = FixedDt * 1000.0;                  // 0.01 s -> 10 ms
    const double BudgetPct = (BudgetMs > 0.0) ? (LastStepAvgMs / BudgetMs) * 100.0 : 0.0;
    const FColor StatsColor = (LastStepAvgMs > BudgetMs) ? FColor::Red : FColor::Cyan;

    GEngine->AddOnScreenDebugMessage(
        StatsDebugKey, 0.f, StatsColor,
        FString::Printf(TEXT("[SimStats] Step avg %.3f ms | max %.3f ms | Budget %.0f%% | Dropped %.2f s"),
            LastStepAvgMs, LastStepMaxMs, BudgetPct, TotalDropped));
}


