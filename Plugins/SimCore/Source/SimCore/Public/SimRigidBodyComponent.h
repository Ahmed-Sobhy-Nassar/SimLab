#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimRigidBodyComponent.generated.h"

class USimClockSubsystem;

UCLASS(ClassGroup = (Simulation), meta = (BlueprintSpawnableComponent))
class SIMCORE_API USimRigidBodyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USimRigidBodyComponent();

    /** Mass in kg */
    UPROPERTY(EditAnywhere, Category = "Sim", meta = (ClampMin = "0.001"))
    double Mass = 1.0;

    /** Velocity at Play start, m/s, world frame */
    UPROPERTY(EditAnywhere, Category = "Sim")
    FVector InitialVelocity = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "Sim")
    bool bUseGravity = true;

    /** Logs range, flight time and impact angle when the body falls back to its start height */
    UPROPERTY(EditAnywhere, Category = "Sim|Debug")
    bool bLogLanding = false;


    /** Logs position, velocity and error vs the exact gravity-only solution */
    UPROPERTY(EditAnywhere, Category = "Sim|Debug")
    bool bLogState = false;

    /** Log once every N steps (10 = every 0.1 s of sim time) */
    UPROPERTY(EditAnywhere, Category = "Sim|Debug", meta = (EditCondition = "bLogState", ClampMin = "1"))
    int32 LogEverySteps = 10;

    /** Adds a force (N, world frame) for the next step only */
    void AddForce(const FVector& ForceN) { PendingForce += ForceN; }

    FVector GetPosition() const { return Position; }   // m
    FVector GetVelocity() const { return Velocity; }   // m/s

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    void OnSimStep(double Dt);
    void OnSimRender(double Alpha);
    void CheckLanding(double Dt);
    void LogState(double Dt) const;
    static constexpr double CmPerMeter = 100.0;
    static constexpr double GravityZ = -9.81;   // m/s^2, Unreal Z is up

    FVector Position = FVector::ZeroVector;   // m
    FVector PrevPosition = FVector::ZeroVector;   // m, for render interpolation
    FVector Velocity = FVector::ZeroVector;   // m/s
    FVector PendingForce = FVector::ZeroVector;   // N, cleared every step
    FVector StartPosition = FVector::ZeroVector;   // m

    int64 ElapsedSteps = 0;
    bool  bLanded = false;

    TWeakObjectPtr<USimClockSubsystem> Clock;
    FDelegateHandle StepHandle;
    FDelegateHandle RenderHandle;
};