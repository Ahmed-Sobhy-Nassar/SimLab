#include "SimTestBall.h"
#include "SimClockSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

ASimTestBall::ASimTestBall()
{
    PrimaryActorTick.bCanEverTick = false;   // no Tick at all: all motion comes from the clock

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded())
    {
        Mesh->SetStaticMesh(Sphere.Object);
    }
}

void ASimTestBall::BeginPlay()
{
    Super::BeginPlay();
    StartLocation = GetActorLocation();

    Clock = GetWorld()->GetSubsystem<USimClockSubsystem>();
    if (Clock.IsValid())
    {
        StepHandle = Clock->OnSimStep.AddUObject(this, &ASimTestBall::OnSimStep);
    }
}

void ASimTestBall::EndPlay(const EEndPlayReason::Type Reason)
{
    // Must unsubscribe, or the clock will call into a destroyed actor
    if (Clock.IsValid())
    {
        Clock->OnSimStep.Remove(StepHandle);
    }
    Super::EndPlay(Reason);
}

void ASimTestBall::OnSimStep(double Dt)
{
    // Semi-implicit Euler with a fixed Dt = same result on any machine
    Velocity += Acceleration * Dt;
    Distance += Velocity * Dt;

    // meters -> centimeters (Unreal units)
    SetActorLocation(StartLocation + FVector(0.0, 0.0, Distance * 100.0));

    if (!Clock.IsValid()) return;

    const int64  Step  = Clock->GetStepCount() + 1;   // the clock hasn't incremented yet
    const double T     = Step * Dt;
    const double Exact = 0.5 * Acceleration * T * T;

    // once per sim second (every 100 steps)
    if (Step % 100 == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Ball] t=%.2fs | Sim=%.4f m | Exact=%.4f m | Error=%.4f m"),
            T, Distance, Exact, Distance - Exact);
    }
}
