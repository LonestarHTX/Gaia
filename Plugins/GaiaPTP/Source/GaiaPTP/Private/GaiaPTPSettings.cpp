#include "GaiaPTPSettings.h"

UGaiaPTPSettings::UGaiaPTPSettings()
{
    // Defaults from Implementation_Guide.md (Constants Reference)
    PlanetRadiusKm = 6370.0f;            // R
    TimeStepMy = 2.0f;                   // δt

    NumSamplePoints = 500000;            // default sphere sampling
    DebugDrawStride = 50;                // draw 1 of N points in debug overlays
    bAllowDynamicResample = true;
    InitialSeed = 1337;
    NumPlates = 40;                      // typical initial plate count
    ContinentalRatio = 0.3f;             // 30% continental

    HighestOceanicRidgeElevationKm = -1.0f; // zᵣ (km)
    AbyssalPlainElevationKm = -6.0f;        // zₐ (km)
    OceanicTrenchElevationKm = -10.0f;      // zₜ (km)
    HighestContinentalAltitudeKm = 10.0f;   // zc (km)

    SubductionDistanceKm = 1800.0f;      // rₛ
    CollisionDistanceKm = 4200.0f;       // rᶜ

    CollisionCoefficient = 1.3e-5f;      // Δᶜ (km⁻¹; stored as scalar)
    MaxPlateSpeedMmPerYear = 100.0f;     // v₀
    OceanicElevationDampening = 4.0e-2f; // εₒ
    ContinentalErosion = 3.0e-5f;        // εᶜ
    SedimentAccretion = 3.0e-1f;         // εf
    SubductionUplift = 6.0e-7f;          // u₀
}
