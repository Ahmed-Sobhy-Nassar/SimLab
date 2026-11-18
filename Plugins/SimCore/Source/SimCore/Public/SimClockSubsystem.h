#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SimClockSubsystem.generated.h"

// Anyone who wants an update every fixed step subscribes here.
// SimCore never needs to know who uses it (drone, vehicle, sensor...).
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSimStep, double /*Dt*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSimRender, double /*Alpha*/);


/**
 * Simulation clock: turns the variable frame DeltaTime into fixed steps.
 * Created automatically with every Game / PIE world.
 */
UCLASS()
class SIMCORE_API USimClockSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- UWorldSubsystem ----
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // ---- FTickableGameObject ----
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(USimClockSubsystem, STATGROUP_Tickables);
    }

    // ---- Control ----
    UFUNCTION(BlueprintCallable, Category = "Sim")
    void SetTimeScale(double InScale) { TimeScale = FMath::Max(0.0, InScale); }

    UFUNCTION(BlueprintCallable, Category = "Sim")
    void SetPaused(bool bInPaused) { bPaused = bInPaused; }

    UFUNCTION(BlueprintCallable, Category = "Sim")
    void ResetClock();

    // ---- Read ----
    UFUNCTION(BlueprintPure, Category = "Sim")
    double GetSimTime() const { return SimTime; }

    UFUNCTION(BlueprintPure, Category = "Sim")
    double GetFixedDt() const { return FixedDt; }

    UFUNCTION(BlueprintPure, Category = "Sim")
    double GetRTF() const { return WallTime > 0.0 ? SimTime / WallTime : 0.0; }

    int64 GetStepCount() const { return StepCount; }

    /** Fired once per fixed step, with the fixed Dt. */
    FOnSimStep OnSimStep;
	FOnSimRender OnSimRender;
	UFUNCTION(BlueprintPure, Category = "Sim")
	double GetAlpha() const { return FixedDt > 0.0 ? Accumulator / FixedDt : 0.0; }

private:
    void StepSimulation(double Dt);
    void DrawDebug() const;
    double LastWallClock = 0.0;   // real clock at the previous frame
    double FixedDt = 0.01;          // 100 Hz
    int32  MaxStepsPerFrame = 10;   // spiral-of-death protection

    double Accumulator = 0.0;       // the "bucket" of unspent time
    double TimeScale = 1.0;
    bool   bPaused = false;
    

    double SimTime = 0.0;           // simulated seconds
    double WallTime = 0.0;          // real seconds (while not paused)
    int64  StepCount = 0;
    int32  LastFrameSteps = 0;



    // ---- Step stats (real time, per window) ----
    static constexpr double StatsWindowSeconds = 1.0;   // length of one stats window

    double StatsWindowStart = 0.0;   // OS clock time when the current window started
    double StepTimeSum = 0.0;   // sum of step durations in this window (s)
    double StepTimeMax = 0.0;   // longest step in this window (s)
    int32  StepTimeCount = 0;     // number of steps in this window

    double LastStepAvgMs = 0.0;      // results of the last finished window, for display
    double LastStepMaxMs = 0.0;

    double TotalDropped = 0.0;       // all sim time dropped since Play started (s)

    static constexpr int32 ClockDebugKey = 1001;   // [SimClock] line
    static constexpr int32 StatsDebugKey = 1002;   // [SimStats] line
};
