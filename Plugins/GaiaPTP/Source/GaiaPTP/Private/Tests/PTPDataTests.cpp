#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TectonicData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPTPDataDefaultsTest, "GaiaPTP.Data.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPTPDataDefaultsTest::RunTest(const FString& Parameters)
{
    FCrustData Crust;
    TestTrue(TEXT("Default oceanic type"), Crust.Type == ECrustType::Oceanic);
    TestTrue(TEXT("Default elevation approx -6"), FMath::IsNearlyEqual(Crust.Elevation, -6.0f));
    TestTrue(TEXT("Default thickness"), FMath::IsNearlyEqual(Crust.Thickness, 7.0f));

    FTerrane Terrane;
    TestEqual(TEXT("Terrane default id"), Terrane.TerraneId, -1);

    FTectonicPlate Plate;
    TestEqual(TEXT("Plate default id"), Plate.PlateId, -1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPTPPlateVelocityTest, "GaiaPTP.Data.PlateVelocity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPTPPlateVelocityTest::RunTest(const FString& Parameters)
{
    FTectonicPlate Plate;
    Plate.RotationAxis = FVector::UpVector;   // Z axis
    Plate.AngularVelocity = 1.0f;             // rad/My (unit)

    const float Radius = 100.0f;              // km
    const FVector P(Radius, 0.0f, 0.0f);

    const FVector V = Plate.GetVelocityAtPoint(P);
    const float ExpectedMag = Radius; // |ω x p| with |ω|=1 and |p|=R => R
    TestTrue(TEXT("Velocity magnitude matches R"), FMath::IsNearlyEqual(V.Size(), ExpectedMag, 1e-3f));
    TestTrue(TEXT("Velocity direction along +Y"), V.Y > 0.0f && FMath::IsNearlyZero(V.X) && FMath::IsNearlyZero(V.Z));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

