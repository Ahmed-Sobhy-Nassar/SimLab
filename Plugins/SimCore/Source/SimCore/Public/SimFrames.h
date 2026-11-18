#pragma once

#include "CoreMinimal.h"

/**
 * Axis conversions between SimCore's Unreal frame and ENU / NED.
 * All vectors are in meters (or m/s). cm <-> m conversion is done at the boundaries, not here.
 *
 * SimCore convention (same as AirSim):
 *   Unreal X = North, Unreal Y = East, Unreal Z = Up
 */
namespace SimFrames
{
    // Unreal (N, E, Up) <-> NED (N, E, Down): flip Z
    inline FVector UnrealToNED(const FVector& V) { return FVector(V.X, V.Y, -V.Z); }
    inline FVector NEDToUnreal(const FVector& V) { return FVector(V.X, V.Y, -V.Z); }

    // Unreal (N, E, Up) <-> ENU (E, N, Up): swap X and Y
    inline FVector UnrealToENU(const FVector& V) { return FVector(V.Y, V.X, V.Z); }
    inline FVector ENUToUnreal(const FVector& V) { return FVector(V.Y, V.X, V.Z); }

    // ENU <-> NED: swap X and Y, flip Z
    inline FVector ENUToNED(const FVector& V) { return FVector(V.Y, V.X, -V.Z); }
    inline FVector NEDToENU(const FVector& V) { return FVector(V.Y, V.X, -V.Z); }
}   