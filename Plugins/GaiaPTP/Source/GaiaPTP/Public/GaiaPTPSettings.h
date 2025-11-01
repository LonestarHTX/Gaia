#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GaiaPTPSettings.generated.h"

/**
 * Global configuration for Procedural Tectonic Planets (values from the paper's Appendix A).
 * Units:
 *  - Distances/elevations in kilometers (km)
 *  - Time step in million years (My)
 *  - Rates in mm/year where applicable
 */
UCLASS(Config=Game, DefaultConfig, MinimalAPI, meta=(DisplayName="PTP Settings"))
class UGaiaPTPSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UGaiaPTPSettings();

    // UDeveloperSettings
    virtual FName GetCategoryName() const override { return TEXT("Gaia"); }

    // --- Planet & Sampling ---
    /** Planet radius (R) in km - used for simulation physics */
    UPROPERTY(EditAnywhere, Config, Category="Planet")
    float PlanetRadiusKm;

    /**
     * Visualization scale: km-to-UU conversion factor.
     * - 1.0 = 1 km → 1 cm (tiny planet, 6.37m radius for Earth)
     * - 100.0 = 1 km → 1 m (small planet, 6.37km radius for Earth) [Default]
     * - 100000.0 = 1:1 real scale (1 km → 100000 cm, Earth = 6370 km radius)
     * Note: UE5 uses 1 UU = 1 cm
     */
    UPROPERTY(EditAnywhere, Config, Category="Planet|Visualization", meta=(ClampMin="0.01"))
    float VisualizationScale;

    /** Simulation time step (δt) in million years */
    UPROPERTY(EditAnywhere, Config, Category="Simulation")
    float TimeStepMy;

    /** Default number of sample points on the sphere */
    UPROPERTY(EditAnywhere, Config, Category="Sampling")
    int32 NumSamplePoints;

    /** Draw every Nth point/edge in debug views to reduce viewport cost (simulation still uses full data). */
    UPROPERTY(EditAnywhere, Config, Category="Sampling", meta=(ClampMin="1"))
    int32 DebugDrawStride;

    /** Allow runtime resampling when NumSamplePoints changes (will recreate planet state). */
    UPROPERTY(EditAnywhere, Config, Category="Sampling")
    bool bAllowDynamicResample;

    /** Global seed for deterministic operations (debug/testing). */
    UPROPERTY(EditAnywhere, Config, Category="Simulation")
    int32 InitialSeed;

    /** Target number of tectonic plates */
    UPROPERTY(EditAnywhere, Config, Category="Plates")
    int32 NumPlates;

    /** Fraction of continental crust [0..1] */
    UPROPERTY(EditAnywhere, Config, Category="Plates", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ContinentalRatio;

    // --- Elevation reference levels (km) ---
    UPROPERTY(EditAnywhere, Config, Category="Elevations")
    float HighestOceanicRidgeElevationKm; // zᵣ

    UPROPERTY(EditAnywhere, Config, Category="Elevations")
    float AbyssalPlainElevationKm; // zₐ

    UPROPERTY(EditAnywhere, Config, Category="Elevations")
    float OceanicTrenchElevationKm; // zₜ

    UPROPERTY(EditAnywhere, Config, Category="Elevations")
    float HighestContinentalAltitudeKm; // zc

    // --- Interaction distances (km) ---
    UPROPERTY(EditAnywhere, Config, Category="Interactions")
    float SubductionDistanceKm; // rₛ

    UPROPERTY(EditAnywhere, Config, Category="Interactions")
    float CollisionDistanceKm; // rᶜ

    // --- Rates (mm/year) ---
    UPROPERTY(EditAnywhere, Config, Category="Rates")
    float CollisionCoefficient; // Δᶜ (km⁻¹ in paper; leave here as scalar)

    UPROPERTY(EditAnywhere, Config, Category="Rates")
    float MaxPlateSpeedMmPerYear; // v₀

    UPROPERTY(EditAnywhere, Config, Category="Rates")
    float OceanicElevationDampening; // εₒ

    UPROPERTY(EditAnywhere, Config, Category="Rates")
    float ContinentalErosion; // εᶜ

    UPROPERTY(EditAnywhere, Config, Category="Rates")
    float SedimentAccretion; // εf

    UPROPERTY(EditAnywhere, Config, Category="Rates")
    float SubductionUplift; // u₀
};
