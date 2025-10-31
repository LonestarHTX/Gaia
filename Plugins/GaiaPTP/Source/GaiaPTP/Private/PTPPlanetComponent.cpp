#include "PTPPlanetComponent.h"
#include "GaiaPTPSettings.h"
#include "FibonacciSphere.h"
#include "TectonicSeeding.h"
#include "TectonicData.h"
#include "PTPProfiling.h"

UPTPPlanetComponent::UPTPPlanetComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bUseProjectDefaults = true;

    // Safe initial defaults; overwritten by project settings on register if enabled
    PlanetRadiusKm = 6370.0f;
    NumSamplePoints = 500000;
    DebugDrawStride = 50;
    NumPlates = 40;
    ContinentalRatio = 0.3f;
    HighestOceanicRidgeElevationKm = -1.0f;
    AbyssalPlainElevationKm = -6.0f;
    OceanicTrenchElevationKm = -10.0f;
    HighestContinentalAltitudeKm = 10.0f;
    SubductionDistanceKm = 1800.0f;
    CollisionDistanceKm = 4200.0f;
    CollisionCoefficient = 1.3e-5f;
    MaxPlateSpeedMmPerYear = 100.0f;
    OceanicElevationDampening = 4.0e-2f;
    ContinentalErosion = 3.0e-5f;
    SedimentAccretion = 3.0e-1f;
    SubductionUplift = 6.0e-7f;
    NumGeneratedPoints = 0;
}

void UPTPPlanetComponent::OnRegister()
{
    Super::OnRegister();
    if (bUseProjectDefaults)
    {
        ApplyDefaultsFromProjectSettings();
    }
}

void UPTPPlanetComponent::ApplyDefaultsFromProjectSettings()
{
    const UGaiaPTPSettings* Settings = GetDefault<UGaiaPTPSettings>();
    if (!Settings) return;

    PlanetRadiusKm = Settings->PlanetRadiusKm;
    NumSamplePoints = Settings->NumSamplePoints;
    DebugDrawStride = Settings->DebugDrawStride;
    NumPlates = Settings->NumPlates;
    ContinentalRatio = Settings->ContinentalRatio;
    HighestOceanicRidgeElevationKm = Settings->HighestOceanicRidgeElevationKm;
    AbyssalPlainElevationKm = Settings->AbyssalPlainElevationKm;
    OceanicTrenchElevationKm = Settings->OceanicTrenchElevationKm;
    HighestContinentalAltitudeKm = Settings->HighestContinentalAltitudeKm;
    SubductionDistanceKm = Settings->SubductionDistanceKm;
    CollisionDistanceKm = Settings->CollisionDistanceKm;
    CollisionCoefficient = Settings->CollisionCoefficient;
    MaxPlateSpeedMmPerYear = Settings->MaxPlateSpeedMmPerYear;
    OceanicElevationDampening = Settings->OceanicElevationDampening;
    ContinentalErosion = Settings->ContinentalErosion;
    SedimentAccretion = Settings->SedimentAccretion;
    SubductionUplift = Settings->SubductionUplift;
}

void UPTPPlanetComponent::RebuildPlanet()
{
    {
        CSV_SCOPED_TIMING_STAT(GAIA_PTP, Sampling);
        SamplePoints.Reset();
        FFibonacciSphere::GeneratePoints(NumSamplePoints, PlanetRadiusKm, SamplePoints);
    }
    NumGeneratedPoints = SamplePoints.Num();

    // Optional: seed plates immediately using simple Voronoi on sphere
    TArray<FVector> Seeds;
    {
        CSV_SCOPED_TIMING_STAT(GAIA_PTP, Seeding);
        FTectonicSeeding::GeneratePlateSeeds(NumPlates, Seeds);
    }

    TArray<int32> PointToPlate;
    TArray<TArray<int32>> PlateToPoints;
    {
        CSV_SCOPED_TIMING_STAT(GAIA_PTP, SeedingAssign);
        FTectonicSeeding::AssignPointsToSeeds(SamplePoints, Seeds, PointToPlate, PlateToPoints);
    }

    // Build plate structs
    TArray<FTectonicPlate> NewPlates;
    NewPlates.SetNum(NumPlates);
    for (int32 p = 0; p < NumPlates; ++p)
    {
        NewPlates[p].PlateId = p;
        NewPlates[p].PointIndices = PlateToPoints[p];
        NewPlates[p].CentroidDir = Seeds.IsValidIndex(p) ? Seeds[p] : FVector::ZeroVector;
    }

    // Store on component (transient preview only for Phase 1)
    Plates = MoveTemp(NewPlates);
    PointPlateIds = MoveTemp(PointToPlate);
}

// Adjacency building will be added in a later step using the CGAL module.
