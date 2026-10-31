#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimTestBall.generated.h"

class USimClockSubsystem;
class UStaticMeshComponent;

/**
 * Test actor: moves with constant acceleration, computed inside the fixed step (not Tick).
 * Compares the simulated distance with the exact law (0.5 * a * t^2).
 */
UCLASS()
class ASimTestBall : public AActor
{
    GENERATED_BODY()

public:
    ASimTestBall();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Mesh;

    /** Acceleration in m/s^2 */
    UPROPERTY(EditAnywhere, Category = "Sim")
    double Acceleration = 2.0;

private:
    void OnSimStep(double Dt);
	void OnSimRender(double Alpha);
    UPROPERTY(EditAnywhere, Category = "Sim")
    bool bInterpolate = true;
    double Velocity = 0.0;   // m/s
    double Distance = 0.0;   // current
	double PrevDistance = 0.0; // prev
    FVector StartLocation;

    TWeakObjectPtr<USimClockSubsystem> Clock;
    FDelegateHandle StepHandle;
	FDelegateHandle RenderHandle;
};
