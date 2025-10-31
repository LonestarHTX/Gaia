#include "FibonacciSphere.h"

void FFibonacciSphere::GeneratePoints(int32 N, float RadiusKm, TArray<FVector>& OutPoints)
{
    OutPoints.Reset(N);
    if (N <= 0)
    {
        return;
    }

    // Golden angle in radians
    const double GoldenAngle = PI * (3.0 - FMath::Sqrt(5.0));

    // Use double precision for accumulation; store as float vectors
    for (int32 i = 0; i < N; ++i)
    {
        const double t = (static_cast<double>(i) + 0.5) / static_cast<double>(N);
        const double y = 1.0 - 2.0 * t;                 // y in [-1,1]
        const double r = FMath::Sqrt(FMath::Max(0.0, 1.0 - y * y));
        const double theta = GoldenAngle * i;
        const double x = r * FMath::Cos(theta);
        const double z = r * FMath::Sin(theta);

        const FVector P(static_cast<float>(x * RadiusKm),
                         static_cast<float>(y * RadiusKm),
                         static_cast<float>(z * RadiusKm));
        OutPoints.Add(P);
    }
}

