#pragma once

#include "CoreMinimal.h"
#include "TectonicData.h"

/**
 * Utilities for initializing crust data and plate dynamics for a new planet.
 * Implements tasks 1.11-1.14 from the implementation guide.
 */
class GAIAPTP_API FCrustInitialization
{
public:
    /**
     * Task 1.11-1.12: Initialize crust data for all sample points.
     *
     * For each point:
     * - Classify as oceanic (70%) or continental (30%) based on ContinentalRatio
     * - Set thickness: oceanic 7km, continental 35km
     * - Set elevation: oceanic varies by distance to ridge, continental ~0.5km
     * - Set age: oceanic 0-200My linear falloff from ridge, continental 500-3000My random
     *
     * @param SamplePoints - Sphere sample positions (input)
     * @param PlateToPoints - Which points belong to each plate (input)
     * @param ContinentalRatio - Fraction of points that should be continental (input)
     * @param AbyssalPlainElevationKm - Base oceanic elevation (input)
     * @param HighestOceanicRidgeElevationKm - Ridge peak elevation (input)
     * @param Seed - Random seed for deterministic results (input)
     * @param OutCrustData - Initialized crust data (output)
     */
    static void InitializeCrustData(
        const TArray<FVector>& SamplePoints,
        const TArray<TArray<int32>>& PlateToPoints,
        float ContinentalRatio,
        float AbyssalPlainElevationKm,
        float HighestOceanicRidgeElevationKm,
        int32 Seed,
        TArray<FCrustData>& OutCrustData
    );

    /**
     * Task 1.13: Initialize plate dynamics (rotation axes and angular velocities).
     *
     * For each plate:
     * - Generate random rotation axis (normalized unit vector)
     * - Generate random angular velocity within max speed constraint
     * - Ensure no plate exceeds MaxPlateSpeedMmPerYear
     *
     * @param NumPlates - Number of plates (input)
     * @param PlanetRadiusKm - Planet radius for velocity constraint (input)
     * @param MaxPlateSpeedMmPerYear - Maximum plate speed in mm/year (input)
     * @param Seed - Random seed for deterministic results (input)
     * @param OutPlates - Plates with initialized rotation axes and velocities (output)
     */
    static void InitializePlateDynamics(
        int32 NumPlates,
        float PlanetRadiusKm,
        float MaxPlateSpeedMmPerYear,
        int32 Seed,
        TArray<FTectonicPlate>& OutPlates
    );

    /**
     * Task 1.14: Detect and mark plate boundary points.
     *
     * A point is on a boundary if any of its neighbors belong to a different plate.
     * Uses adjacency data from Delaunay triangulation.
     *
     * @param PointPlateIds - Which plate each point belongs to (input)
     * @param Neighbors - Adjacency list for each point (input)
     * @param OutIsBoundaryPoint - Boolean flag per point (output)
     */
    static void DetectPlateBoundaries(
        const TArray<int32>& PointPlateIds,
        const TArray<TArray<int32>>& Neighbors,
        TArray<bool>& OutIsBoundaryPoint
    );

private:
    // Helper: Classify a plate as oceanic or continental based on random selection
    static void ClassifyPlates(
        int32 NumPlates,
        float ContinentalRatio,
        int32 Seed,
        TArray<bool>& OutIsPlateContinent
    );

    // Helper: Compute distance from point to plate centroid on sphere (geodesic)
    static float ComputeGeodesicDistanceToCenter(
        const FVector& Point,
        const FVector& PlateCentroid,
        float PlanetRadiusKm
    );
};
