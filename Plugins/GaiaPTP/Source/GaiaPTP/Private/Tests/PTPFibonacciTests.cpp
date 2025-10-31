#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "FibonacciSphere.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFibonacciCountRadiusTest, "GaiaPTP.Fibonacci.CountAndRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFibonacciCountRadiusTest::RunTest(const FString& Parameters)
{
    const int32 N = 20000;
    const float Radius = 100.0f; // km
    TArray<FVector> Points;
    FFibonacciSphere::GeneratePoints(N, Radius, Points);

    TestEqual(TEXT("Exact point count"), Points.Num(), N);

    double MaxErr = 0.0;
    for (const FVector& P : Points)
    {
        const double Err = FMath::Abs(P.Size() - Radius);
        MaxErr = FMath::Max(MaxErr, Err);
    }
    TestTrue(TEXT("Radius within tolerance"), MaxErr < 1e-3 * Radius);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFibonacciUniformityBinsTest, "GaiaPTP.Fibonacci.UniformityBins",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFibonacciUniformityBinsTest::RunTest(const FString& Parameters)
{
    const int32 N = 50000;
    const float Radius = 100.0f; // km
    const int32 K = 20;          // bins along y (equal area stripes when splitting y uniformly)
    const double AllowedFrac = 0.20; // 20% deviation per bin

    TArray<FVector> Points;
    FFibonacciSphere::GeneratePoints(N, Radius, Points);

    TArray<int32> Bins; Bins.Init(0, K);
    for (const FVector& P : Points)
    {
        const FVector Nrm = P.GetSafeNormal();
        const double y = FMath::Clamp<double>(Nrm.Y, -1.0, 1.0);
        int32 Bin = FMath::Clamp(FMath::FloorToInt(((y + 1.0) * 0.5) * K), 0, K - 1);
        ++Bins[Bin];
    }

    const double Expected = static_cast<double>(N) / static_cast<double>(K);
    bool bAllWithin = true;
    for (int32 i = 0; i < K; ++i)
    {
        const double DevFrac = FMath::Abs(static_cast<double>(Bins[i]) - Expected) / Expected;
        if (DevFrac > AllowedFrac)
        {
            AddError(FString::Printf(TEXT("Bin %d deviation %.2f%% exceeds limit"), i, DevFrac * 100.0));
            bAllWithin = false;
        }
    }
    TestTrue(TEXT("Uniformity within 20% per bin"), bAllWithin);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

