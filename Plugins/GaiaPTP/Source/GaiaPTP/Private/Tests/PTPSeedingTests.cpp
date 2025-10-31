#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "FibonacciSphere.h"
#include "TectonicSeeding.h"

static void MakeSmallSphere(int32 N, float R, TArray<FVector>& Out)
{
    FFibonacciSphere::GeneratePoints(N, R, Out);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPTPSeedingBasicTest, "GaiaPTP.Seeding.Basic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPTPSeedingBasicTest::RunTest(const FString& Parameters)
{
    const int32 N = 1000;
    const int32 M = 10;
    TArray<FVector> Points; MakeSmallSphere(N, 1.0f, Points);
    TArray<FVector> Seeds; FTectonicSeeding::GeneratePlateSeeds(M, Seeds);

    TArray<int32> PointToPlate; TArray<TArray<int32>> PlateToPoints;
    FTectonicSeeding::AssignPointsToSeeds(Points, Seeds, PointToPlate, PlateToPoints);

    // All points assigned
    TestEqual(TEXT("PointToPlate size"), PointToPlate.Num(), N);

    // Plate count and non-empty assignment
    TestEqual(TEXT("Plate count"), PlateToPoints.Num(), M);
    int32 NonEmpty = 0; for (const auto& Arr : PlateToPoints) { if (Arr.Num() > 0) ++NonEmpty; }
    TestTrue(TEXT("Most plates non-empty"), NonEmpty >= M - 1);

    // Spot-check: a random point is nearest to its assigned seed by dot product
    const int32 idx = 123;
    const FVector Pn = Points[idx].GetSafeNormal();
    float BestDot = -FLT_MAX; int32 BestIdx = -1;
    for (int32 j = 0; j < M; ++j)
    {
        const float d = FVector::DotProduct(Pn, Seeds[j]);
        if (d > BestDot) { BestDot = d; BestIdx = j; }
    }
    TestEqual(TEXT("Nearest seed matches assignment"), PointToPlate[idx], BestIdx);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
