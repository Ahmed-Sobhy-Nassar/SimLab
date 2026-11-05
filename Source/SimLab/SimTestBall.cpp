#include "SimTestBall.h"
#include "SimClockSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Async/ParallelFor.h"

double ASimTestBall::WorkRange(int32 Begin, int32 End, double Offset)
{
    double Sum = 0.0;
    for (int32 i = Begin; i < End; ++i)
    {
        Sum += FMath::Sqrt(static_cast<double>(i) + Offset);   // each i is independent
    }
    return Sum;
}

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

    PrevDistance = Distance;
    Velocity += Acceleration * Dt;
    Distance += Velocity * Dt;

    if (!Clock.IsValid()) return;

    const int64  Step = Clock->GetStepCount() + 1;
    const double Offset = static_cast<double>(Step) * 1e-3;

    {
        TRACE_CPUPROFILER_EVENT_SCOPE(SimTestBall_FakeWork);

        constexpr int32 NumChunks = 16;   // FIXED, not the core count
        double StepWork = 0.0;

        auto ChunkBegin = [this](int32 Chunk)
            {
                return static_cast<int32>(static_cast<int64>(WorkLoad) * Chunk / NumChunks);
            };

        switch (WorkMode)
        {
            case ESimWorkMode::Serial:
                StepWork = WorkRange(0, WorkLoad, Offset);
                break;

            case ESimWorkMode::Parallel:
            {
                double Partials[NumChunks] = {};
                ParallelFor(NumChunks, [&](int32 Chunk)
                    {
                        // each chunk writes only its own slot
                        Partials[Chunk] = WorkRange(ChunkBegin(Chunk), ChunkBegin(Chunk + 1), Offset);
                    });
                for (int32 Chunk = 0; Chunk < NumChunks; ++Chunk)
                {
                    StepWork += Partials[Chunk];   // combine in a fixed order
                }
                break;
            }
        } 

        TotalWork += StepWork;
    }

    const double T = Step * Dt;
    const double Exact = 0.5 * Acceleration * T * T;

    if (Step % 100 == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Ball] t=%.2fs | Sim=%.4f m | Error=%.4f m | RTF=%.2f | Work=%.17g"),
            T, Distance, Distance - Exact, Clock->GetRTF(), TotalWork);
    }
}


void ASimTestBall::OnSimRender(double Alpha)
{
    const double Shown = bInterpolate
        ? FMath::Lerp(PrevDistance, Distance, Alpha)
        : Distance;
    SetActorLocation(StartLocation + FVector(0.0, 0.0, Shown * 100.0));

}
