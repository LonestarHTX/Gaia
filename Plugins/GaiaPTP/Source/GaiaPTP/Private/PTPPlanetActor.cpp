#include "PTPPlanetActor.h"
#include "PTPPlanetComponent.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshSimple.h"
#include "RealtimeMeshLibrary.h"
#include "PTPProfiling.h"
#include "IPTPAdjacencyProvider.h"

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

        if (URealtimeMeshSimple* RMSimple = RealtimeMesh->InitializeRealtimeMesh<URealtimeMeshSimple>())
        {
            CSV_SCOPED_TIMING_STAT(GAIA_PTP, PreviewBuild);
            const TArray<FVector>& Pts = Planet->SamplePoints;
            const TArray<int32>& PlateIds = Planet->PointPlateIds;
            if (Pts.Num() == 0)
            {
                return;
            }

            // Remove inactive preview group to avoid double rendering
            const FRealtimeMeshSectionGroupKey PointsGroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("PTPPreview"));
            const FRealtimeMeshSectionGroupKey SurfaceGroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("PTPSurface"));

            if (PreviewMode == EPTPPreviewMode::Points)
            {
                RMSimple->RemoveSectionGroup(SurfaceGroupKey);
            }
            else
            {
                RMSimple->RemoveSectionGroup(PointsGroupKey);
            }

            auto PlateColor = [](int32 PlateId) -> FColor
            {
                uint32 h = ::GetTypeHash(PlateId);
                uint8 r = (uint8)((h      ) & 0xFF);
                uint8 g = (uint8)((h >> 8 ) & 0xFF);
                uint8 b = (uint8)((h >> 16) & 0xFF);
                return FColor(r/2 + 64, g/2 + 64, b/2 + 64, 255);
            };

            if (PreviewMode == EPTPPreviewMode::Points)
            {
                const int32 Stride = FMath::Max(1, Planet->DebugDrawStride);
                FRealtimeMeshStreamSet StreamSet;
                RealtimeMesh::TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
                Builder.EnableTangents();
                Builder.EnableColors();

                const float Radius = Planet->PlanetRadiusKm;
                const float Scale = Planet->VisualizationScale;
                const float MarkerSize = Radius * 0.01f; // 1% of radius (pre-scale)

                for (int32 i = 0; i < Pts.Num(); i += Stride)
                {
                    const FVector N = Pts[i].GetSafeNormal(); // Normal from original point
                    const FVector P = Pts[i] * Scale; // Apply visualization scale
                    FVector T = FVector::CrossProduct(N, FVector::UpVector);
                    if (T.IsNearlyZero()) T = FVector::CrossProduct(N, FVector::RightVector);
                    T.Normalize();
                    const FVector B = FVector::CrossProduct(N, T);

                    const FVector V0 = P + ( T + B) * MarkerSize * Scale * 0.5f;
                    const FVector V1 = P + (-T + B) * MarkerSize * Scale * 0.5f;
                    const FVector V2 = P + (-T - B) * MarkerSize * Scale * 0.5f;
                    const FVector V3 = P + ( T - B) * MarkerSize * Scale * 0.5f;

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

                RMSimple->CreateSectionGroup(PointsGroupKey, StreamSet);
            }
            else // Surface
            {
                // Surface mode requires adjacency - use BuildAdjacency() button to generate triangulation
                if (Planet->Triangles.Num() > 0)
                {
                    FRealtimeMeshStreamSet StreamSet;
                    RealtimeMesh::TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
                    Builder.EnableTangents();
                    Builder.EnableColors();

                    const float Scale = Planet->VisualizationScale;

                    // Build vertex buffer once (shared vertices)
                    for (int32 i = 0; i < Pts.Num(); ++i)
                    {
                        const FVector N = Pts[i].GetSafeNormal(); // Normal from original point
                        const FVector P = Pts[i] * Scale; // Apply visualization scale
                        FVector T = FVector::CrossProduct(N, FVector::UpVector);
                        if (T.IsNearlyZero()) T = FVector::CrossProduct(N, FVector::RightVector);
                        T.Normalize();

                        const FColor C = PlateIds.IsValidIndex(i) ? PlateColor(PlateIds[i]) : FColor::Cyan;

                        Builder.AddVertex(FVector3f(P))
                            .SetNormalAndTangent(FVector3f(N), FVector3f(T))
                            .SetColor(C);
                    }

                    // Build index buffer (triangles reference existing vertices)
                    for (const FIntVector& Tri : Planet->Triangles)
                    {
                        const int32 ia = Tri.X; const int32 ib = Tri.Y; const int32 ic = Tri.Z;
                        if (Pts.IsValidIndex(ia) && Pts.IsValidIndex(ib) && Pts.IsValidIndex(ic))
                        {
                            // Front-facing
                            Builder.AddTriangle(ia, ib, ic);
                            // Back-facing (double-sided for preview reliability)
                            Builder.AddTriangle(ic, ib, ia);
                        }
                    }

                    RMSimple->CreateSectionGroup(SurfaceGroupKey, StreamSet);

                    UE_LOG(LogTemp, Log, TEXT("PTP: Surface rendered - %d vertices, %d triangles"),
                        Pts.Num(), Planet->Triangles.Num());
                }
            }
        }
    }
}

void APTPPlanetActor::BuildAdjacency()
{
    if (!Planet)
    {
        return;
    }

    const int32 NumPoints = Planet->SamplePoints.Num();

    // Estimate based on point count
    FString TimeEstimate;
    if (NumPoints < 5000)
        TimeEstimate = TEXT("< 1 second");
    else if (NumPoints < 20000)
        TimeEstimate = TEXT("1-5 seconds");
    else if (NumPoints < 100000)
        TimeEstimate = TEXT("10-30 seconds");
    else
        TimeEstimate = TEXT("1-5 minutes");

    UE_LOG(LogTemp, Warning, TEXT("PTP: Building adjacency for %d points... (estimated: %s)"), NumPoints, *TimeEstimate);

    TSharedPtr<IPTPAdjacencyProvider> Provider = CreateCGALAdjacencyProvider();
    FPTPAdjacency Adj;
    FString Error;
    if (!Provider.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("PTP: No adjacency provider available."));
        return;
    }

    const double StartTime = FPlatformTime::Seconds();
    if (!Provider->Build(Planet->SamplePoints, Adj, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("PTP: Adjacency build failed: %s"), *Error);
        return;
    }
    const double ElapsedTime = FPlatformTime::Seconds() - StartTime;

    UE_LOG(LogTemp, Warning, TEXT("PTP: Adjacency built successfully in %.2f seconds (%d triangles)"), ElapsedTime, Adj.Triangles.Num());

    Planet->Neighbors = MoveTemp(Adj.Neighbors);
    Planet->Triangles = MoveTemp(Adj.Triangles);
    Planet->NumTriangles = Planet->Triangles.Num();
}

void APTPPlanetActor::TogglePreviewMode()
{
    PreviewMode = (PreviewMode == EPTPPreviewMode::Points) ? EPTPPreviewMode::Surface : EPTPPreviewMode::Points;
    // Re-run construction script to refresh preview
    RerunConstructionScripts();
}
