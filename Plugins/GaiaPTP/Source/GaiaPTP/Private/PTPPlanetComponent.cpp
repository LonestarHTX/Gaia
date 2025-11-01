#include "PTPPlanetComponent.h"
#include "GaiaPTPSettings.h"
#include "FibonacciSphere.h"
#include "TectonicSeeding.h"
#include "TectonicData.h"
#include "CrustInitialization.h"
#include "PTPProfiling.h"

UPTPPlanetComponent::UPTPPlanetComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bUseProjectDefaults = true;

    // Safe initial defaults; overwritten by project settings on register if enabled
    PlanetRadiusKm = 6370.0f;
    VisualizationScale = 100.0f;
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
    NumTriangles = 0;
    NumPlatesGenerated = 0;
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
    VisualizationScale = Settings->VisualizationScale;
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

uint32 UPTPPlanetComponent::ComputeSettingsHash() const
{
    uint32 Hash = 0;
    Hash = HashCombine(Hash, GetTypeHash(NumSamplePoints));
    Hash = HashCombine(Hash, GetTypeHash(NumPlates));
    Hash = HashCombine(Hash, GetTypeHash(PlanetRadiusKm));
    Hash = HashCombine(Hash, GetTypeHash(VisualizationScale));
    return Hash;
}

void UPTPPlanetComponent::RebuildPlanet()
{
    // Skip rebuild if settings haven't changed (optimization for OnConstruction spam)
    const uint32 CurrentHash = ComputeSettingsHash();
    if (CurrentHash == CachedSettingsHash && SamplePoints.Num() > 0)
    {
        // Data already generated and settings unchanged - skip rebuild
        return;
    }

    {
        CSV_SCOPED_TIMING_STAT(GAIA_PTP, Sampling);
        SamplePoints.Reset();
        FFibonacciSphere::GeneratePoints(NumSamplePoints, PlanetRadiusKm, SamplePoints);
    }
    NumGeneratedPoints = SamplePoints.Num();

    // Update cached hash
    CachedSettingsHash = CurrentHash;

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

    // Task 1.11-1.12: Initialize crust data
    TArray<FCrustData> NewCrustData;
    {
        CSV_SCOPED_TIMING_STAT(GAIA_PTP, CrustInit);

        const UGaiaPTPSettings* Settings = GetDefault<UGaiaPTPSettings>();
        FCrustInitialization::InitializeCrustData(
            SamplePoints,
            PlateToPoints,
            ContinentalRatio,
            Settings->AbyssalPlainElevationKm,
            Settings->HighestOceanicRidgeElevationKm,
            Settings->InitialSeed,
            NewCrustData
        );
    }

    // Task 1.13: Initialize plate dynamics
    {
        CSV_SCOPED_TIMING_STAT(GAIA_PTP, PlateDynamics);

        const UGaiaPTPSettings* Settings = GetDefault<UGaiaPTPSettings>();
        FCrustInitialization::InitializePlateDynamics(
            NumPlates,
            PlanetRadiusKm,
            MaxPlateSpeedMmPerYear,
            Settings->InitialSeed + 100, // Offset seed
            NewPlates
        );
    }

    // Store on component (transient preview only for Phase 1)
    Plates = MoveTemp(NewPlates);
    PointPlateIds = MoveTemp(PointToPlate);
    CrustData = MoveTemp(NewCrustData);
    NumPlatesGenerated = Plates.Num();
}

// Adjacency building will be added in a later step using the CGAL module.
