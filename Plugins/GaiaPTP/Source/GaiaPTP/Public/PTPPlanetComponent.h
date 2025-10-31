#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PTPPlanetComponent.generated.h"

/** Component holding per-planet settings and data; prefers actor-local overrides. */
UCLASS(ClassGroup=(Gaia), meta=(BlueprintSpawnableComponent))
class GAIAPTP_API UPTPPlanetComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPTPPlanetComponent();

    // Whether to initialize properties from project-wide defaults on registration.
    UPROPERTY(EditAnywhere, Category="PTP|Config")
    bool bUseProjectDefaults;

    // --- Planet & Sampling ---
    UPROPERTY(EditAnywhere, Category="PTP|Planet")
    float PlanetRadiusKm;

    UPROPERTY(EditAnywhere, Category="PTP|Sampling")
    int32 NumSamplePoints;

    UPROPERTY(EditAnywhere, Category="PTP|Sampling", meta=(ClampMin="1"))
    int32 DebugDrawStride;

    UPROPERTY(EditAnywhere, Category="PTP|Plates")
    int32 NumPlates;

    UPROPERTY(EditAnywhere, Category="PTP|Plates", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ContinentalRatio;

    // --- Elevations (km) ---
    UPROPERTY(EditAnywhere, Category="PTP|Elevations")
    float HighestOceanicRidgeElevationKm;

    UPROPERTY(EditAnywhere, Category="PTP|Elevations")
    float AbyssalPlainElevationKm;

    UPROPERTY(EditAnywhere, Category="PTP|Elevations")
    float OceanicTrenchElevationKm;

    UPROPERTY(EditAnywhere, Category="PTP|Elevations")
    float HighestContinentalAltitudeKm;

    // --- Distances (km) ---
    UPROPERTY(EditAnywhere, Category="PTP|Interactions")
    float SubductionDistanceKm;

    UPROPERTY(EditAnywhere, Category="PTP|Interactions")
    float CollisionDistanceKm;

    // --- Rates (mm/yr) ---
    UPROPERTY(EditAnywhere, Category="PTP|Rates")
    float CollisionCoefficient;

    UPROPERTY(EditAnywhere, Category="PTP|Rates")
    float MaxPlateSpeedMmPerYear;

    UPROPERTY(EditAnywhere, Category="PTP|Rates")
    float OceanicElevationDampening;

    UPROPERTY(EditAnywhere, Category="PTP|Rates")
    float ContinentalErosion;

    UPROPERTY(EditAnywhere, Category="PTP|Rates")
    float SedimentAccretion;

    UPROPERTY(EditAnywhere, Category="PTP|Rates")
    float SubductionUplift;

    // Generated data (preview only for Phase 1)
    UPROPERTY(VisibleAnywhere, Transient, Category="PTP|Debug")
    int32 NumGeneratedPoints;

    // Not saved; preview cloud only
    TArray<FVector> SamplePoints;

    // Mapping from sample index -> plate id (preview)
    UPROPERTY(VisibleAnywhere, Transient, Category="PTP|Debug")
    TArray<int32> PointPlateIds;

    // Plate data (preview)
    UPROPERTY(VisibleAnywhere, Transient, Category="PTP|Debug")
    TArray<struct FTectonicPlate> Plates;

    // Adjacency (preview)
    // Not exposed due to nested array UPROPERTY limitation
    TArray<TArray<int32>> Neighbors;

    UPROPERTY(VisibleAnywhere, Transient, Category="PTP|Debug")
    TArray<FIntVector> Triangles;

public:
    UFUNCTION(BlueprintCallable, Category="PTP")
    void ApplyDefaultsFromProjectSettings();

    UFUNCTION(BlueprintCallable, Category="PTP")
    void RebuildPlanet();

    // Adjacency building is provided by the CGAL module tests and tools; will be wired later for editor usage.

protected:
    virtual void OnRegister() override;
};
