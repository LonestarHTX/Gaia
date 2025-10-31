#include "PTPPlanetActor.h"
#include "PTPPlanetComponent.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshSimple.h"
#include "RealtimeMeshLibrary.h"
#include "PTPProfiling.h"

using namespace RealtimeMesh;
#include "Core/RealtimeMeshBuilder.h"

APTPPlanetActor::APTPPlanetActor()
{
    PrimaryActorTick.bCanEverTick = false;
    RealtimeMesh = CreateDefaultSubobject<URealtimeMeshComponent>(TEXT("RealtimeMesh"));
    SetRootComponent(RealtimeMesh);

    Planet = CreateDefaultSubobject<UPTPPlanetComponent>(TEXT("Planet"));
}

void APTPPlanetActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (Planet)
    {
        Planet->RebuildPlanet();

        // Build simple debug preview (Task 1.7): draw quads for a subset of points colored by plate id
        if (URealtimeMeshSimple* RMSimple = RealtimeMesh->InitializeRealtimeMesh<URealtimeMeshSimple>())
        {
            CSV_SCOPED_TIMING_STAT(GAIA_PTP, PreviewBuild);
            const int32 Stride = FMath::Max(1, Planet->DebugDrawStride);
            const TArray<FVector>& Pts = Planet->SamplePoints;
            const TArray<int32>& PlateIds = Planet->PointPlateIds;
            if (Pts.Num() > 0)
            {
                FRealtimeMeshStreamSet StreamSet;
                // Use 32-bit indices to avoid overflow when many markers are drawn
                RealtimeMesh::TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
                Builder.EnableTangents();
                Builder.EnableColors();

                auto PlateColor = [](int32 PlateId) -> FColor
                {
                    // Stable hash to color mapping
                    uint32 h = ::GetTypeHash(PlateId);
                    uint8 r = (uint8)((h      ) & 0xFF);
                    uint8 g = (uint8)((h >> 8 ) & 0xFF);
                    uint8 b = (uint8)((h >> 16) & 0xFF);
                    // Desaturate a bit for readability
                    return FColor(r/2 + 64, g/2 + 64, b/2 + 64, 255);
                };

                const float Radius = Planet->PlanetRadiusKm;
                const float MarkerSize = Radius * 0.01f; // 1% of radius

                TArray<int32> VertexIndexForPoint; VertexIndexForPoint.SetNumUninitialized(Pts.Num());

                for (int32 i = 0; i < Pts.Num(); i += Stride)
                {
                    const FVector P = Pts[i];
                    const FVector N = P.GetSafeNormal();
                    FVector T = FVector::CrossProduct(N, FVector::UpVector);
                    if (T.IsNearlyZero()) T = FVector::CrossProduct(N, FVector::RightVector);
                    T.Normalize();
                    const FVector B = FVector::CrossProduct(N, T);

                    const FVector V0 = P + ( T + B) * MarkerSize * 0.5f;
                    const FVector V1 = P + (-T + B) * MarkerSize * 0.5f;
                    const FVector V2 = P + (-T - B) * MarkerSize * 0.5f;
                    const FVector V3 = P + ( T - B) * MarkerSize * 0.5f;

                    const FVector3f Nf = FVector3f(N);
                    const FVector3f Tf = FVector3f(T);
                    const FColor C = PlateIds.IsValidIndex(i) ? PlateColor(PlateIds[i]) : FColor::Cyan;

                    int32 i0 = Builder.AddVertex(FVector3f(V0)).SetNormalAndTangent(Nf, Tf).SetColor(C);
                    int32 i1 = Builder.AddVertex(FVector3f(V1)).SetNormalAndTangent(Nf, Tf).SetColor(C);
                    int32 i2 = Builder.AddVertex(FVector3f(V2)).SetNormalAndTangent(Nf, Tf).SetColor(C);
                    int32 i3 = Builder.AddVertex(FVector3f(V3)).SetNormalAndTangent(Nf, Tf).SetColor(C);

                    // Front-facing
                    Builder.AddTriangle(i0, i1, i2);
                    Builder.AddTriangle(i0, i2, i3);
                    // Back-facing (double-sided so all markers remain visible)
                    Builder.AddTriangle(i2, i1, i0);
                    Builder.AddTriangle(i3, i2, i0);
                }

                const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("PTPPreview"));
                // Create or update the group (CreateSectionGroup handles both cases)
                RMSimple->CreateSectionGroup(GroupKey, StreamSet);
            }
        }
    }
}
