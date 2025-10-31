#include "TectonicSeeding.h"
#include "FibonacciSphere.h"

void FTectonicSeeding::GeneratePlateSeeds(int32 NumPlates, TArray<FVector>& OutSeeds)
{
    OutSeeds.Reset(NumPlates);
    if (NumPlates <= 0)
    {
        return;
    }
    // Use Fibonacci to distribute seeds, then normalize to unit sphere
    TArray<FVector> Temp;
    const float Radius = 1.0f;
    FFibonacciSphere::GeneratePoints(NumPlates, Radius, Temp);
    OutSeeds.Reserve(NumPlates);
    for (const FVector& P : Temp)
    {
        OutSeeds.Add(P.GetSafeNormal());
    }
}

void FTectonicSeeding::AssignPointsToSeeds(const TArray<FVector>& Points,
                                           const TArray<FVector>& Seeds,
                                           TArray<int32>& OutPointToPlate,
                                           TArray<TArray<int32>>& OutPlateToPoints)
{
    const int32 N = Points.Num();
    const int32 M = Seeds.Num();
    OutPointToPlate.SetNumUninitialized(N);
    OutPlateToPoints.SetNum(M);
    for (int32 j = 0; j < M; ++j) OutPlateToPoints[j].Reset();

    // Assign by maximizing the dot product with normalized seed direction (equivalent to minimizing great-circle distance)
    for (int32 i = 0; i < N; ++i)
    {
        const FVector Pn = Points[i].GetSafeNormal();
        int32 BestIdx = 0;
        float BestDot = -FLT_MAX;
        for (int32 j = 0; j < M; ++j)
        {
            const float D = FVector::DotProduct(Pn, Seeds[j]);
            if (D > BestDot)
            {
                BestDot = D;
                BestIdx = j;
            }
        }
        OutPointToPlate[i] = BestIdx;
        OutPlateToPoints[BestIdx].Add(i);
    }
}

