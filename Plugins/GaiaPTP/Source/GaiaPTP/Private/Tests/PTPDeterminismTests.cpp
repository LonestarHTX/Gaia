#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "FibonacciSphere.h"
#include "TectonicSeeding.h"

static uint64 HashPoints(const TArray<FVector>& P)
{
    uint64 H = 1469598103934665603ull; // FNV-1a 64-bit offset basis
    for (const FVector& V : P)
    {
        const uint64* Ptr = reinterpret_cast<const uint64*>(&V);
        H ^= Ptr[0]; H *= 1099511628211ull;
        H ^= Ptr[1]; H *= 1099511628211ull;
    }
    return H;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPTPSamplingDeterminismTest, "GaiaPTP.Determinism.Sampling",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPTPSamplingDeterminismTest::RunTest(const FString& Parameters)
{
    TArray<FVector> A,B; FFibonacciSphere::GeneratePoints(50000, 123.0f, A); FFibonacciSphere::GeneratePoints(50000, 123.0f, B);
    TestEqual(TEXT("Same inputs yield same number"), A.Num(), B.Num());
    TestEqual(TEXT("Hash equal"), HashPoints(A), HashPoints(B));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPTPSeedingDeterminismTest, "GaiaPTP.Determinism.Seeding",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPTPSeedingDeterminismTest::RunTest(const FString& Parameters)
{
    TArray<FVector> Points; FFibonacciSphere::GeneratePoints(10000, 1.0f, Points);
    TArray<FVector> Seeds; FTectonicSeeding::GeneratePlateSeeds(20, Seeds);

    TArray<int32> MapA; TArray<TArray<int32>> RevA;
    TArray<int32> MapB; TArray<TArray<int32>> RevB;
    FTectonicSeeding::AssignPointsToSeeds(Points, Seeds, MapA, RevA);
    FTectonicSeeding::AssignPointsToSeeds(Points, Seeds, MapB, RevB);

    TestEqual(TEXT("Same size"), MapA.Num(), MapB.Num());
    for (int32 i=0;i<MapA.Num();++i)
    {
        if (MapA[i] != MapB[i]) { AddError(TEXT("Non-deterministic mapping")); return false; }
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

