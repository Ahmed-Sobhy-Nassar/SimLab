#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SimClockSubsystem.generated.h"

// Anyone who wants an update every fixed step subscribes here.
// SimCore never needs to know who uses it (drone, vehicle, sensor...).
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSimStep, double /*Dt*/);

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

private:
    void StepSimulation(double Dt);
    void DrawDebug() const;

    double FixedDt = 0.01;          // 100 Hz
    int32  MaxStepsPerFrame = 10;   // spiral-of-death protection

    double Accumulator = 0.0;       // the "bucket" of unspent time
    double TimeScale = 1.0;
    bool   bPaused = false;

    double SimTime = 0.0;           // simulated seconds
    double WallTime = 0.0;          // real seconds (while not paused)
    int64  StepCount = 0;
    int32  LastFrameSteps = 0;
};
