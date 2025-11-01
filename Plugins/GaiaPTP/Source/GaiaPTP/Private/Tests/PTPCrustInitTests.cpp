#include "Misc/AutomationTest.h"
#include "CrustInitialization.h"
#include "FibonacciSphere.h"
#include "TectonicSeeding.h"
#include "GaiaPTPSettings.h"
#include "PTPPlanetComponent.h"

// Test Task 1.11-1.12: Crust data initialization
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPTPCrustDataInitTest, "GaiaPTP.CrustInit.DataInit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FPTPCrustDataInitTest::RunTest(const FString& Parameters)
{
    // Generate test data
    TArray<FVector> SamplePoints;
    FFibonacciSphere::GeneratePoints(1000, 6370.0f, SamplePoints);

    // Create 10 plates
    TArray<FVector> Seeds;
    FTectonicSeeding::GeneratePlateSeeds(10, Seeds);

    TArray<int32> PointToPlate;
    TArray<TArray<int32>> PlateToPoints;
    FTectonicSeeding::AssignPointsToSeeds(SamplePoints, Seeds, PointToPlate, PlateToPoints);

    // Initialize crust data
    TArray<FCrustData> CrustData;
    FCrustInitialization::InitializeCrustData(
        SamplePoints,
        PlateToPoints,
        0.3f, // 30% continental
        -6.0f, // Abyssal plain
        -1.0f, // Oceanic ridge
        12345, // Seed
        CrustData
    );

    // Verify size
    TestEqual(TEXT("CrustData array size matches points"), CrustData.Num(), SamplePoints.Num());

    // Count crust types
    int32 OceanicCount = 0;
    int32 ContinentalCount = 0;
    for (const FCrustData& Crust : CrustData)
    {
        if (Crust.Type == ECrustType::Oceanic)
        {
            OceanicCount++;
            // Verify oceanic properties
            TestEqual(TEXT("Oceanic thickness is 7km"), Crust.Thickness, 7.0f);
            TestTrue(TEXT("Oceanic elevation is negative"), Crust.Elevation < 0.0f);
            TestTrue(TEXT("Oceanic age is valid"), Crust.OceanicAge >= 0.0f && Crust.OceanicAge <= 200.0f);
        }
        else
        {
            ContinentalCount++;
            // Verify continental properties
            TestEqual(TEXT("Continental thickness is 35km"), Crust.Thickness, 35.0f);
            TestTrue(TEXT("Continental elevation is near 0.5km"), FMath::Abs(Crust.Elevation - 0.5f) < 0.3f);
            TestTrue(TEXT("Continental age is old"), Crust.OrogenyAge >= 500.0f && Crust.OrogenyAge <= 3000.0f);
        }
    }

    // Verify continental ratio (allow 10% tolerance due to plate-based classification)
    float ActualContinentalRatio = float(ContinentalCount) / CrustData.Num();
    TestTrue(TEXT("Continental ratio near 30%"), FMath::Abs(ActualContinentalRatio - 0.3f) < 0.15f);

    return true;
}

// Test Task 1.13: Plate dynamics initialization
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPTPPlateDynamicsTest, "GaiaPTP.CrustInit.PlateDynamics", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FPTPPlateDynamicsTest::RunTest(const FString& Parameters)
{
    // Create 10 plates
    TArray<FTectonicPlate> Plates;
    Plates.SetNum(10);
    for (int32 i = 0; i < 10; ++i)
    {
        Plates[i].PlateId = i;
    }

    // Initialize dynamics
    const float PlanetRadiusKm = 6370.0f;
    const float MaxSpeedMmPerYear = 100.0f;
    FCrustInitialization::InitializePlateDynamics(
        10,
        PlanetRadiusKm,
        MaxSpeedMmPerYear,
        12345,
        Plates
    );

    // Convert max speed to km/My then to angular velocity
    // Note: 1 mm/year == 1 km/My (e.g., 100 mm/year = 100 km/My)
    const float MaxSpeedKmPerMy = MaxSpeedMmPerYear; // km/My
    const float MaxAngularVelocity = MaxSpeedKmPerMy / PlanetRadiusKm;

    for (const FTectonicPlate& Plate : Plates)
    {
        // Verify rotation axis is normalized
        TestTrue(TEXT("Rotation axis is normalized"),
            FMath::Abs(Plate.RotationAxis.Size() - 1.0f) < 0.001f);

        // Verify angular velocity is within bounds
        TestTrue(TEXT("Angular velocity within max"),
            FMath::Abs(Plate.AngularVelocity) <= MaxAngularVelocity + 0.001f);

        // Verify velocity at equator doesn't exceed max speed
        // Point on equator perpendicular to rotation axis
        FVector TestPoint = FVector::CrossProduct(Plate.RotationAxis, FVector::UpVector);
        if (TestPoint.IsNearlyZero())
        {
            TestPoint = FVector::CrossProduct(Plate.RotationAxis, FVector::RightVector);
        }
        TestPoint = TestPoint.GetSafeNormal() * PlanetRadiusKm;

        FVector VelocityKmPerMy = Plate.GetVelocityAtPoint(TestPoint);
        float SpeedKmPerMy = VelocityKmPerMy.Size();
        TestTrue(TEXT("Surface velocity within max"),
            SpeedKmPerMy <= MaxSpeedKmPerMy + 0.01f);
    }

    return true;
}

// Test Task 1.14: Boundary detection
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPTPBoundaryDetectionTest, "GaiaPTP.CrustInit.BoundaryDetection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FPTPBoundaryDetectionTest::RunTest(const FString& Parameters)
{
    // Create simple test case with known boundaries
    // 6 points in 2 plates (3 each)
    TArray<int32> PointPlateIds = {0, 0, 0, 1, 1, 1};

    // Adjacency: points 2 and 3 are neighbors (boundary), all others internal
    TArray<TArray<int32>> Neighbors;
    Neighbors.SetNum(6);
    Neighbors[0] = {1};          // Internal to plate 0
    Neighbors[1] = {0, 2};       // Internal to plate 0
    Neighbors[2] = {1, 3};       // BOUNDARY: plate 0 neighbors with plate 1
    Neighbors[3] = {2, 4};       // BOUNDARY: plate 1 neighbors with plate 0
    Neighbors[4] = {3, 5};       // Internal to plate 1
    Neighbors[5] = {4};          // Internal to plate 1

    // Detect boundaries
    TArray<bool> IsBoundaryPoint;
    FCrustInitialization::DetectPlateBoundaries(
        PointPlateIds,
        Neighbors,
        IsBoundaryPoint
    );

    // Verify
    TestEqual(TEXT("IsBoundaryPoint array size"), IsBoundaryPoint.Num(), 6);
    TestFalse(TEXT("Point 0 not boundary"), IsBoundaryPoint[0]);
    TestFalse(TEXT("Point 1 not boundary"), IsBoundaryPoint[1]);
    TestTrue(TEXT("Point 2 is boundary"), IsBoundaryPoint[2]);
    TestTrue(TEXT("Point 3 is boundary"), IsBoundaryPoint[3]);
    TestFalse(TEXT("Point 4 not boundary"), IsBoundaryPoint[4]);
    TestFalse(TEXT("Point 5 not boundary"), IsBoundaryPoint[5]);

    return true;
}

// Test Task 1.11-1.14: Integration test with real component
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPTPCrustInitIntegrationTest, "GaiaPTP.CrustInit.Integration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FPTPCrustInitIntegrationTest::RunTest(const FString& Parameters)
{
    // Create standalone component (no world needed for this test)
    UPTPPlanetComponent* Planet = NewObject<UPTPPlanetComponent>();
    TestNotNull(TEXT("Component created"), Planet);

    if (!Planet)
    {
        return false;
    }

    // Configure for small test
    Planet->NumSamplePoints = 1000;
    Planet->NumPlates = 10;
    Planet->ContinentalRatio = 0.3f;
    Planet->PlanetRadiusKm = 6370.0f;

    // Rebuild planet (should initialize crust data)
    Planet->RebuildPlanet();

    // Verify crust data initialized
    TestEqual(TEXT("CrustData initialized"), Planet->CrustData.Num(), Planet->SamplePoints.Num());
    TestTrue(TEXT("At least some crust data"), Planet->CrustData.Num() > 0);

    if (Planet->CrustData.Num() > 0)
    {
        // Check a sample point
        const FCrustData& Sample = Planet->CrustData[0];
        TestTrue(TEXT("Thickness is positive"), Sample.Thickness > 0.0f);
        TestTrue(TEXT("Type is valid"), Sample.Type == ECrustType::Oceanic || Sample.Type == ECrustType::Continental);
    }

    // Verify plate dynamics initialized
    TestEqual(TEXT("Plates initialized"), Planet->Plates.Num(), 10);
    if (Planet->Plates.Num() > 0)
    {
        const FTectonicPlate& Plate = Planet->Plates[0];
        TestTrue(TEXT("Rotation axis normalized"), FMath::Abs(Plate.RotationAxis.Size() - 1.0f) < 0.01f);
        TestTrue(TEXT("Angular velocity set"), Plate.AngularVelocity != 0.0f);
    }

    return true;
}
