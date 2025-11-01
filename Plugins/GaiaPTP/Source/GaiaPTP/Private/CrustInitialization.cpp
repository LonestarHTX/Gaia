#include "CrustInitialization.h"
#include "Math/RandomStream.h"
#include "Async/ParallelFor.h"
#include "GaiaPTP.h"
#include "HAL/PlatformTime.h"

void FCrustInitialization::InitializeCrustData(
    const TArray<FVector>& SamplePoints,
    const TArray<TArray<int32>>& PlateToPoints,
    float ContinentalRatio,
    float AbyssalPlainElevationKm,
    float HighestOceanicRidgeElevationKm,
    int32 Seed,
    TArray<FCrustData>& OutCrustData
)
{
    const int32 NumPoints = SamplePoints.Num();
    const int32 NumPlates = PlateToPoints.Num();

    OutCrustData.SetNum(NumPoints);

    // Step 1: Classify each plate as oceanic or continental
    TArray<bool> IsPlateContinent;
    ClassifyPlates(NumPlates, ContinentalRatio, Seed, IsPlateContinent);

    FRandomStream RandStream(Seed + 1); // Offset seed for different random sequence

    // Step 2: Initialize crust data for each plate (parallelizable)
    const bool bDoParallel = IConsoleManager::Get().FindConsoleVariable(TEXT("ptp.parallel"))
        ? IConsoleManager::Get().FindConsoleVariable(TEXT("ptp.parallel"))->GetInt() != 0
        : true;

    auto WorkPerPlate = [&](int32 PlateIdx)
    {
        const bool bIsContinental = IsPlateContinent[PlateIdx];
        const TArray<int32>& PlatePoints = PlateToPoints[PlateIdx];

        // Deterministic per-plate RNG (order independent)
        FRandomStream LocalRand(Seed + 1000 + PlateIdx * 10007);

        // Compute plate centroid for distance calculations
        FVector PlateCentroid = FVector::ZeroVector;
        for (int32 PointIdx : PlatePoints)
        {
            PlateCentroid += SamplePoints[PointIdx];
        }
        if (PlatePoints.Num() > 0)
        {
            PlateCentroid /= PlatePoints.Num();
            PlateCentroid.Normalize();
        }

        for (int32 PointIdx : PlatePoints)
        {
            FCrustData& Crust = OutCrustData[PointIdx];

            if (bIsContinental)
            {
                // Continental crust
                Crust.Type = ECrustType::Continental;
                Crust.Thickness = 35.0f; // km
                Crust.Elevation = 0.5f + LocalRand.FRandRange(-0.2f, 0.2f); // ~0.5km with variation
                Crust.OrogenyAge = LocalRand.FRandRange(500.0f, 3000.0f); // 500-3000 My
                Crust.OrogenyType = EOrogenyType::None; // Set during collisions later
                Crust.FoldDirection = FVector::ZeroVector; // Set during collisions later

                // Reset oceanic fields
                Crust.OceanicAge = 0.0f;
                Crust.RidgeDirection = FVector::ZeroVector;
            }
            else
            {
                // Oceanic crust
                Crust.Type = ECrustType::Oceanic;
                Crust.Thickness = 7.0f; // km

                // Elevation varies linearly from ridge (center) to abyssal plain (edge)
                // Distance from plate center determines age and elevation
                const FVector& Point = SamplePoints[PointIdx];
                const float DistanceAngle = FMath::Acos(FMath::Clamp(FVector::DotProduct(Point, PlateCentroid), -1.0f, 1.0f));
                const float MaxAngle = PI / 4.0f; // Assume max distance is ~45 degrees
                const float NormalizedDist = FMath::Clamp(DistanceAngle / MaxAngle, 0.0f, 1.0f);

                // Elevation: ridge at center (-1 km), abyssal plain at edge (-6 km)
                Crust.Elevation = FMath::Lerp(HighestOceanicRidgeElevationKm, AbyssalPlainElevationKm, NormalizedDist);

                // Age: 0 My at ridge, 200 My at edge (linear falloff)
                Crust.OceanicAge = NormalizedDist * 200.0f;

                // Ridge direction: perpendicular to direction from center (will be refined with boundaries)
                FVector ToCenter = PlateCentroid - Point;
                ToCenter.Normalize();
                FVector Perpendicular = FVector::CrossProduct(ToCenter, Point); // Tangent on sphere
                Crust.RidgeDirection = Perpendicular.GetSafeNormal();

                // Reset continental fields
                Crust.OrogenyAge = 0.0f;
                Crust.OrogenyType = EOrogenyType::None;
                Crust.FoldDirection = FVector::ZeroVector;
            }
        }
    };

    const double StartTime = FPlatformTime::Seconds();
    if (bDoParallel)
    {
        ParallelFor(NumPlates, WorkPerPlate);
        const double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
        UE_LOG(LogGaiaPTP, Log, TEXT("Crust init: %d plates parallelized in %.2fms"), NumPlates, ElapsedMs);
    }
    else
    {
        for (int32 PlateIdx = 0; PlateIdx < NumPlates; ++PlateIdx)
        {
            WorkPerPlate(PlateIdx);
        }
        const double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
        UE_LOG(LogGaiaPTP, Log, TEXT("Crust init: %d plates sequential in %.2fms"), NumPlates, ElapsedMs);
    }
}

void FCrustInitialization::InitializePlateDynamics(
    int32 NumPlates,
    float PlanetRadiusKm,
    float MaxPlateSpeedMmPerYear,
    int32 Seed,
    TArray<FTectonicPlate>& OutPlates
)
{
    // Convert max speed from mm/year to km/My
    // Note: 1 mm/year == 1 km/My (mm -> km is 1e-6, years -> My is 1e6)
    // Example: 100 mm/year = 100 km/My
    const float MaxSpeedKmPerMy = MaxPlateSpeedMmPerYear; // mm/year -> km/My (numerically identical)

    // Convert to max angular velocity: ω_max = v_max / R (radians per My)
    const float MaxAngularVelocity = MaxSpeedKmPerMy / PlanetRadiusKm;

    FRandomStream RandStream(Seed);

    for (int32 PlateIdx = 0; PlateIdx < OutPlates.Num(); ++PlateIdx)
    {
        FTectonicPlate& Plate = OutPlates[PlateIdx];

        // Generate random rotation axis (unit vector in 3D space)
        FVector RandomAxis;
        do
        {
            RandomAxis = FVector(
                RandStream.FRandRange(-1.0f, 1.0f),
                RandStream.FRandRange(-1.0f, 1.0f),
                RandStream.FRandRange(-1.0f, 1.0f)
            );
        } while (RandomAxis.SizeSquared() < 0.01f); // Avoid near-zero vectors

        Plate.RotationAxis = RandomAxis.GetSafeNormal();

        // Generate random angular velocity within constraints
        // Use uniform distribution between -MaxAngularVelocity and +MaxAngularVelocity
        Plate.AngularVelocity = RandStream.FRandRange(-MaxAngularVelocity, MaxAngularVelocity);
    }
}

void FCrustInitialization::DetectPlateBoundaries(
    const TArray<int32>& PointPlateIds,
    const TArray<TArray<int32>>& Neighbors,
    TArray<bool>& OutIsBoundaryPoint
)
{
    const int32 NumPoints = PointPlateIds.Num();
    OutIsBoundaryPoint.SetNumZeroed(NumPoints);

    const bool bDoParallel = IConsoleManager::Get().FindConsoleVariable(TEXT("ptp.parallel"))
        ? IConsoleManager::Get().FindConsoleVariable(TEXT("ptp.parallel"))->GetInt() != 0
        : true;

    auto WorkPerPoint = [&](int32 PointIdx)
    {
        const int32 MyPlateId = PointPlateIds[PointIdx];
        bool bBoundary = false;
        if (Neighbors.IsValidIndex(PointIdx))
        {
            for (int32 NeighborIdx : Neighbors[PointIdx])
            {
                if (PointPlateIds.IsValidIndex(NeighborIdx))
                {
                    if (PointPlateIds[NeighborIdx] != MyPlateId)
                    {
                        bBoundary = true;
                        break;
                    }
                }
            }
        }
        OutIsBoundaryPoint[PointIdx] = bBoundary;
    };

    const double StartTime = FPlatformTime::Seconds();
    if (bDoParallel)
    {
        ParallelFor(NumPoints, WorkPerPoint);
        const double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
        UE_LOG(LogGaiaPTP, Log, TEXT("Boundary detection: %d points parallelized in %.2fms"), NumPoints, ElapsedMs);
    }
    else
    {
        for (int32 i = 0; i < NumPoints; ++i) { WorkPerPoint(i); }
        const double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
        UE_LOG(LogGaiaPTP, Log, TEXT("Boundary detection: %d points sequential in %.2fms"), NumPoints, ElapsedMs);
    }
}

void FCrustInitialization::ClassifyPlates(
    int32 NumPlates,
    float ContinentalRatio,
    int32 Seed,
    TArray<bool>& OutIsPlateContinent
)
{
    OutIsPlateContinent.SetNum(NumPlates);

    // Determine how many plates should be continental
    const int32 NumContinental = FMath::RoundToInt(NumPlates * ContinentalRatio);

    // Randomly select which plates are continental
    FRandomStream RandStream(Seed);
    TArray<int32> PlateIndices;
    for (int32 i = 0; i < NumPlates; ++i)
    {
        PlateIndices.Add(i);
    }

    // Shuffle using Fisher-Yates
    for (int32 i = NumPlates - 1; i > 0; --i)
    {
        int32 j = RandStream.RandRange(0, i);
        PlateIndices.Swap(i, j);
    }

    // First NumContinental plates are continental, rest are oceanic
    for (int32 i = 0; i < NumPlates; ++i)
    {
        OutIsPlateContinent[PlateIndices[i]] = (i < NumContinental);
    }
}

float FCrustInitialization::ComputeGeodesicDistanceToCenter(
    const FVector& Point,
    const FVector& PlateCentroid,
    float PlanetRadiusKm
)
{
    // Geodesic distance on sphere: d = R * arccos(dot(A, B))
    const float CosAngle = FMath::Clamp(FVector::DotProduct(Point, PlateCentroid), -1.0f, 1.0f);
    const float Angle = FMath::Acos(CosAngle);
    return PlanetRadiusKm * Angle;
}
