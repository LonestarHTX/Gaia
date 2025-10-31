#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "IPTPAdjacencyProvider.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPTPAdjacencySmokeTest, "GaiaPTP.Adjacency.Smoke",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPTPAdjacencySmokeTest::RunTest(const FString& Parameters)
{
    auto Provider = CreateCGALAdjacencyProvider();
    // Local simple generator (avoid cross-module linkage on tests)
    TArray<FVector> Points; Points.Reserve(2000);
    const double GoldenAngle = PI * (3.0 - FMath::Sqrt(5.0));
    const int32 N = 2000; const double R = 1.0;
    for (int32 i = 0; i < N; ++i)
    {
        const double t = (i + 0.5) / static_cast<double>(N);
        const double y = 1.0 - 2.0 * t;
        const double r = FMath::Sqrt(FMath::Max(0.0, 1.0 - y * y));
        const double theta = GoldenAngle * i;
        const double x = r * FMath::Cos(theta);
        const double z = r * FMath::Sin(theta);
        Points.Add(FVector((float)(x*R), (float)(y*R), (float)(z*R)));
    }
    FPTPAdjacency Adj; FString Err;
    const bool bOk = Provider.IsValid() && Provider->Build(Points, Adj, Err);

#if WITH_PTP_CGAL_LIB
    if (!bOk)
    {
        AddError(FString::Printf(TEXT("CGAL adjacency not available: %s"), *Err));
        return false;
    }
    TestTrue(TEXT("Has neighbors"), Adj.Neighbors.Num() == Points.Num());
    TestTrue(TEXT("Has triangles"), Adj.Triangles.Num() > 0);
#else
    AddWarning(TEXT("CGAL not configured; adjacency test skipped."));
#endif
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPTPAdjacencyIntegrityTest, "GaiaPTP.Adjacency.Integrity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPTPAdjacencyIntegrityTest::RunTest(const FString& Parameters)
{
    auto Provider = CreateCGALAdjacencyProvider();
    TArray<FVector> Points; Points.Reserve(4000);
    const double GoldenAngle = PI * (3.0 - FMath::Sqrt(5.0));
    const int32 N = 4000; const double R = 1.0;
    for (int32 i = 0; i < N; ++i)
    {
        const double t = (i + 0.5) / static_cast<double>(N);
        const double y = 1.0 - 2.0 * t;
        const double r = FMath::Sqrt(FMath::Max(0.0, 1.0 - y * y));
        const double theta = GoldenAngle * i;
        Points.Add(FVector((float)(r*FMath::Cos(theta)*R), (float)(y*R), (float)(r*FMath::Sin(theta)*R)));
    }

    FPTPAdjacency Adj; FString Err;
    const bool bOk = Provider.IsValid() && Provider->Build(Points, Adj, Err);

#if WITH_PTP_CGAL_LIB
    if (!bOk) { AddError(Err); return false; }
    // Basic invariants
    TestEqual(TEXT("Neighbor array size"), Adj.Neighbors.Num(), N);
    int32 WithNeighbors=0; int64 DegreeSum=0;
    for (int32 i=0;i<N;++i)
    {
        const TArray<int32>& Nbs = Adj.Neighbors[i];
        for (int32 v : Nbs)
        {
            if (v<0 || v>=N) { AddError(TEXT("Neighbor index out of range")); return false; }
            if (v==i) { AddError(TEXT("Self neighbor found")); return false; }
        }
        if (Nbs.Num()>0) { ++WithNeighbors; DegreeSum += Nbs.Num(); }
    }
    TestTrue(TEXT("Most points have neighbors"), WithNeighbors > N*0.95);
    TestTrue(TEXT("Average degree reasonable"), (DegreeSum/ (double)N) >= 3.0);
#else
    AddWarning(TEXT("CGAL not configured; integrity test skipped."));
#endif
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
