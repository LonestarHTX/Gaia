#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GaiaPTPSettings.h"
#include "PTPPlanetComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPTPSettingsDefaultsTest, "GaiaPTP.Settings.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPTPSettingsDefaultsTest::RunTest(const FString& Parameters)
{
    const UGaiaPTPSettings* Settings = GetDefault<UGaiaPTPSettings>();
    TestNotNull(TEXT("Settings asset exists"), Settings);
    if (!Settings) return false;

    auto NearlyEqual = [](double A, double B){ return FMath::Abs(A - B) < 1e-6; };

    TestTrue(TEXT("Planet radius default"), NearlyEqual(Settings->PlanetRadiusKm, 6370.0));
    TestTrue(TEXT("Time step default"), NearlyEqual(Settings->TimeStepMy, 2.0));
    TestEqual(TEXT("Default sample points"), Settings->NumSamplePoints, 500000);
    TestTrue(TEXT("Abyssal elevation default"), NearlyEqual(Settings->AbyssalPlainElevationKm, -6.0));
    TestTrue(TEXT("Trench elevation default"), NearlyEqual(Settings->OceanicTrenchElevationKm, -10.0));
    TestTrue(TEXT("Collision distance default"), NearlyEqual(Settings->CollisionDistanceKm, 4200.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPTPComponentDefaultsCopyTest, "GaiaPTP.Component.DefaultsCopied",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPTPComponentDefaultsCopyTest::RunTest(const FString& Parameters)
{
    UPTPPlanetComponent* Comp = NewObject<UPTPPlanetComponent>();
    TestNotNull(TEXT("Component created"), Comp);
    if (!Comp) return false;
    Comp->bUseProjectDefaults = true;
    Comp->ApplyDefaultsFromProjectSettings();

    const UGaiaPTPSettings* Settings = GetDefault<UGaiaPTPSettings>();
    TestTrue(TEXT("Radius copied"), FMath::IsNearlyEqual(Comp->PlanetRadiusKm, Settings->PlanetRadiusKm));
    TestEqual(TEXT("Points copied"), Comp->NumSamplePoints, Settings->NumSamplePoints);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
