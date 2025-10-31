#pragma once

#include "CoreMinimal.h"
#include "FibonacciSphere.generated.h"

/** Utility: generate near-uniform points on a sphere using a golden-angle spiral. */
USTRUCT()
struct FFibonacciSphere
{
    GENERATED_BODY()

    /**
     * Generate N points on a sphere of given radius (km). Points are returned in world-space units (km).
     * The center is at the origin.
     */
    static void GeneratePoints(int32 N, float RadiusKm, TArray<FVector>& OutPoints);
};

