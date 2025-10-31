#pragma once

#include "CoreMinimal.h"

/** Utilities for seeding plates and assigning points via spherical Voronoi. */
struct FTectonicSeeding
{
    /**
     * Generate NumPlates seed directions distributed on the unit sphere.
     * Uses Fibonacci sampling for near-uniform distribution.
     */
    static void GeneratePlateSeeds(int32 NumPlates, TArray<FVector>& OutSeeds);

    /**
     * Assign each point to the closest seed by geodesic distance (max dot product).
     * Returns mapping Point->PlateId and Plate->PointIndices.
     */
    static void AssignPointsToSeeds(const TArray<FVector>& Points,
                                    const TArray<FVector>& Seeds,
                                    TArray<int32>& OutPointToPlate,
                                    TArray<TArray<int32>>& OutPlateToPoints);
};

