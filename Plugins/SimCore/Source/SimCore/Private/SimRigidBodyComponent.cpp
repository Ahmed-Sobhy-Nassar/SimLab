#include "SimRigidBodyComponent.h"
#include "SimClockSubsystem.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "SimFrames.h"

USimRigidBodyComponent::USimRigidBodyComponent()
{
    PrimaryComponentTick.bCanEverTick = false;   // moved by the sim clock, not Tick
}

void USimRigidBodyComponent::BeginPlay()
{
    Super::BeginPlay();

    // cm -> m, once, at the boundary
    StartPosition = GetOwner()->GetActorLocation() / CmPerMeter;
    Position = StartPosition;
    PrevPosition = StartPosition;
    Velocity = InitialVelocity;

    // round-trip check: converting there and back must give the same vector
    const FVector Probe(1.0, 2.0, 3.0);
    const bool bFramesOk =
        SimFrames::NEDToUnreal(SimFrames::UnrealToNED(Probe)).Equals(Probe) &&
        SimFrames::ENUToUnreal(SimFrames::UnrealToENU(Probe)).Equals(Probe) &&
        SimFrames::NEDToENU(SimFrames::ENUToNED(Probe)).Equals(Probe) &&
        SimFrames::ENUToNED(SimFrames::UnrealToENU(Probe)).Equals(SimFrames::UnrealToNED(Probe));
    UE_LOG(LogTemp, Log, TEXT("[Frames] Round-trip check: %s"), bFramesOk ? TEXT("OK") : TEXT("FAILED"));

    Clock = GetWorld()->GetSubsystem<USimClockSubsystem>();
    if (Clock.IsValid())
    {
        StepHandle = Clock->OnSimStep.AddUObject(this, &USimRigidBodyComponent::OnSimStep);
        RenderHandle = Clock->OnSimRender.AddUObject(this, &USimRigidBodyComponent::OnSimRender);
    }
}

void USimRigidBodyComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Clock.IsValid())
    {
        Clock->OnSimStep.Remove(StepHandle);
        Clock->OnSimRender.Remove(RenderHandle);
    }
    Super::EndPlay(Reason);
}

void USimRigidBodyComponent::OnSimStep(double Dt)
{
    // 1. sum all forces (N)
    FVector Force = PendingForce;
    if (bUseGravity)
    {
        Force += FVector(0.0, 0.0, GravityZ) * Mass;
    }
    PendingForce = FVector::ZeroVector;

    // 2. Newton: a = F / m
    const FVector Acceleration = Force / Mass;

    // 3. semi-implicit Euler
    PrevPosition = Position;
    Velocity += Acceleration * Dt;
    Position += Velocity * Dt;
    ++ElapsedSteps;

    if (bLogState && ElapsedSteps % LogEverySteps == 0)
    {
        LogState(Dt);
    }

    if (bLogLanding && !bLanded)
    {
        CheckLanding(Dt);
    }
}

void USimRigidBodyComponent::OnSimRender(double Alpha)
{
    // m -> cm, once, at the boundary
    const FVector Shown = FMath::Lerp(PrevPosition, Position, Alpha);
    GetOwner()->SetActorLocation(Shown * CmPerMeter);
}

void USimRigidBodyComponent::CheckLanding(double Dt)
{
    const double StartZ = StartPosition.Z;

    // only when crossing the start height going down
    if (!(PrevPosition.Z > StartZ && Position.Z <= StartZ))
    {
        return;
    }

    // where inside this step did we cross? (0..1)
    const double  Frac = (PrevPosition.Z - StartZ) / (PrevPosition.Z - Position.Z);
    const FVector Landing = FMath::Lerp(PrevPosition, Position, Frac);
    const double  FlightTime = (ElapsedSteps - 1 + Frac) * Dt;
    const double  Range = FVector::Dist2D(StartPosition, Landing);

    // impact angle from the dot product
    const FVector VelDir = Velocity.GetSafeNormal();
    const FVector HorizDir = FVector(Velocity.X, Velocity.Y, 0.0).GetSafeNormal();
    const double  CosAngle = FMath::Clamp(FVector::DotProduct(VelDir, HorizDir), -1.0, 1.0);
    const double  ImpactDeg = FMath::RadiansToDegrees(FMath::Acos(CosAngle));

    // exact projectile law
    const double G = -GravityZ;
    const double Speed = InitialVelocity.Size();
    const double Theta = FMath::Atan2(InitialVelocity.Z, InitialVelocity.Size2D());
    const double ExactRange = Speed * Speed * FMath::Sin(2.0 * Theta) / G;
    const double ExactTime = 2.0 * Speed * FMath::Sin(Theta) / G;

    UE_LOG(LogTemp, Warning,
        TEXT("[Projectile] Range %.4f m (exact %.4f) | Time %.4f s (exact %.4f) | Impact %.2f deg"),
        Range, ExactRange, FlightTime, ExactTime, ImpactDeg);

    bLanded = true;
}

void USimRigidBodyComponent::LogState(double Dt) const
{
    const double  T = ElapsedSteps * Dt;
    const FVector GravityAccel = bUseGravity ? FVector(0.0, 0.0, GravityZ) : FVector::ZeroVector;

    // exact solution, valid only when gravity is the only force
    const FVector Exact = StartPosition + InitialVelocity * T + GravityAccel * (0.5 * T * T);
    const FVector Error = Position - Exact;

    UE_LOG(LogTemp, Log,
        TEXT("[RigidBody] t=%.2fs | Pos (%.3f, %.3f, %.3f) m | Vel (%.3f, %.3f, %.3f) m/s | Speed %.3f m/s | Err %.4f m"),
        T,
        Position.X, Position.Y, Position.Z,
        Velocity.X, Velocity.Y, Velocity.Z,
        Velocity.Size(),
        Error.Size());

    const FVector PosNED = SimFrames::UnrealToNED(Position);
    const FVector VelNED = SimFrames::UnrealToNED(Velocity);
    const FVector PosENU = SimFrames::UnrealToENU(Position);

    UE_LOG(LogTemp, Log,
        TEXT("[Frames] t=%.2fs | NED pos (%.3f, %.3f, %.3f) vel (%.3f, %.3f, %.3f) | ENU pos (%.3f, %.3f, %.3f)"),
        T,
        PosNED.X, PosNED.Y, PosNED.Z,
        VelNED.X, VelNED.Y, VelNED.Z,
        PosENU.X, PosENU.Y, PosENU.Z);
}