#include "SimTestBall.h"
#include "SimClockSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
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
		RenderHandle = Clock->OnSimRender.AddUObject(this, &ASimTestBall::OnSimRender);
    }
}

void ASimTestBall::EndPlay(const EEndPlayReason::Type Reason)
{
    // Must unsubscribe, or the clock will call into a destroyed actor
    if (Clock.IsValid())
    {
        Clock->OnSimStep.Remove(StepHandle);
		Clock->OnSimRender.Remove(RenderHandle);
    }
    Super::EndPlay(Reason);
}

void ASimTestBall::OnSimStep(double Dt)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(SimTestBall_Step);

    // Semi-implicit Euler with a fixed Dt = same result on any machine
    PrevDistance = Distance;
    Velocity += Acceleration * Dt;
    Distance += Velocity * Dt;


    if (!Clock.IsValid()) return;

    const int64  Step  = Clock->GetStepCount() + 1;   // the clock hasn't incremented yet
    const double T     = Step * Dt;
    const double Exact = 0.5 * Acceleration * T * T;

    // Fake heavy work (like a sensor). Does not touch the physics.
    {
        TRACE_CPUPROFILER_EVENT_SCOPE(SimTestBall_FakeWork);
        for (int32 i = 0; i < WorkLoad; ++i)
        {
            WorkSink += FMath::Sqrt(static_cast<double>(i) + WorkSink * 1e-9);
        }
    }

    // once per sim second (every 100 steps)
    if (Step % 100 == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Ball] t=%.2fs | Sim=%.4f m | Exact=%.4f m | Error=%.4f m"),
            T, Distance, Exact, Distance - Exact);
    }
}


void ASimTestBall::OnSimRender(double Alpha)
{
    const double Shown = bInterpolate
        ? FMath::Lerp(PrevDistance, Distance, Alpha)
        : Distance;
    SetActorLocation(StartLocation + FVector(0.0, 0.0, Shown * 100.0));

}
