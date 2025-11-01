#include "PTPPlanetActor.h"
#include "PTPPlanetComponent.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshSimple.h"
#include "RealtimeMeshLibrary.h"
#include "PTPProfiling.h"
#include "IPTPAdjacencyProvider.h"
#include "CrustInitialization.h"

using namespace RealtimeMesh;
#include "Core/RealtimeMeshBuilder.h"

APTPPlanetActor::APTPPlanetActor()
{
    PrimaryActorTick.bCanEverTick = false;
    RealtimeMesh = CreateDefaultSubobject<URealtimeMeshComponent>(TEXT("RealtimeMesh"));
    SetRootComponent(RealtimeMesh);

    // Disable culling - planet is huge and should always be visible
    RealtimeMesh->SetCullDistance(0.0f); // 0 = never cull
    RealtimeMesh->bUseAsOccluder = false;
    RealtimeMesh->CastShadow = false; // Disable shadows for performance

    Planet = CreateDefaultSubobject<UPTPPlanetComponent>(TEXT("Planet"));

    // Load default planet material (vertex colored)
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
        TEXT("/Game/Materials/Dev/M_DevPlanet")
    );
    if (MaterialFinder.Succeeded())
    {
        PlanetMaterial = MaterialFinder.Object;
    }
}

void APTPPlanetActor::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("PTPPlanetActor: BeginPlay started at location %s"), *GetActorLocation().ToString());

    if (!Planet)
    {
        UE_LOG(LogTemp, Error, TEXT("PTPPlanetActor: No Planet component!"));
        return;
    }

    // Smart rebuild: only regenerate if data is missing or stale
    const bool bNeedsRebuild = (Planet->SamplePoints.Num() != Planet->NumSamplePoints);
    const bool bNeedsAdjacency = (Planet->Triangles.Num() == 0);

    if (bNeedsRebuild)
    {
        UE_LOG(LogTemp, Warning, TEXT("PTP: Rebuilding planet (sample count changed: %d -> %d)"),
            Planet->SamplePoints.Num(), Planet->NumSamplePoints);
        Planet->RebuildPlanet();
        BuildAdjacency(); // Need new adjacency after rebuild
    }
    else if (bNeedsAdjacency)
    {
        UE_LOG(LogTemp, Warning, TEXT("PTP: Building missing adjacency (no triangulation data)"));
        BuildAdjacency();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("PTP: Using cached planet data (%d points, %d triangles) - no rebuild needed"),
            Planet->SamplePoints.Num(), Planet->Triangles.Num());
    }

    // Always rebuild mesh (lightweight operation)
    RebuildMesh();

    UE_LOG(LogTemp, Log, TEXT("PTPPlanetActor: BeginPlay complete - mesh should be visible"));
}

void APTPPlanetActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (!Planet)
    {
        return;
    }

    // Smart rebuild: only regenerate if data is missing or stale
    const bool bNeedsRebuild = (Planet->SamplePoints.Num() != Planet->NumSamplePoints);
    const bool bNeedsAdjacency = (Planet->Triangles.Num() == 0);

    if (bNeedsRebuild)
    {
        UE_LOG(LogTemp, Log, TEXT("PTP: OnConstruction rebuilding planet (sample count changed: %d -> %d)"),
            Planet->SamplePoints.Num(), Planet->NumSamplePoints);
        Planet->RebuildPlanet();
        BuildAdjacency(); // Need new adjacency after rebuild
    }
    else if (bNeedsAdjacency)
    {
        UE_LOG(LogTemp, Log, TEXT("PTP: OnConstruction building missing adjacency"));
        BuildAdjacency();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("PTP: OnConstruction using cached planet data (%d points, %d triangles)"),
            Planet->SamplePoints.Num(), Planet->Triangles.Num());
    }

    // Always rebuild mesh (lightweight operation)
    RebuildMesh();
}

void APTPPlanetActor::RebuildMesh()
{
    if (!Planet)
    {
        return;
    }

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
                // Better hash mixing for small integers (MurmurHash3 finalizer)
                uint32 h = PlateId;
                h = ((h >> 16) ^ h) * 0x45d9f3b;
                h = ((h >> 16) ^ h) * 0x45d9f3b;
                h = (h >> 16) ^ h;

                uint8 r = (uint8)((h      ) & 0xFF);
                uint8 g = (uint8)((h >> 8 ) & 0xFF);
                uint8 b = (uint8)((h >> 16) & 0xFF);

                // Map to pastel range: 64-191 (avoids very dark and very bright)
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

                // Apply material
                if (PlanetMaterial)
                {
                    RealtimeMesh->SetMaterial(0, PlanetMaterial);
                }
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

                    // Apply material
                    if (PlanetMaterial)
                    {
                        RealtimeMesh->SetMaterial(0, PlanetMaterial);
                    }

                    UE_LOG(LogTemp, Log, TEXT("PTP: Surface rendered - %d vertices, %d triangles"),
                        Pts.Num(), Planet->Triangles.Num());
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

    // Task 1.14: Detect plate boundaries
    if (Planet->PointPlateIds.Num() > 0)
    {
        FCrustInitialization::DetectPlateBoundaries(
            Planet->PointPlateIds,
            Planet->Neighbors,
            Planet->IsBoundaryPoint
        );

        // Count boundary points for logging
        int32 BoundaryCount = 0;
        for (bool bIsBoundary : Planet->IsBoundaryPoint)
        {
            if (bIsBoundary) BoundaryCount++;
        }

        UE_LOG(LogTemp, Log, TEXT("PTP: Detected %d boundary points (%.1f%%)"),
            BoundaryCount, 100.0f * BoundaryCount / FMath::Max(1, NumPoints));
    }

    // Refresh mesh to show new triangulation
    RebuildMesh();
}

void APTPPlanetActor::TogglePreviewMode()
{
    PreviewMode = (PreviewMode == EPTPPreviewMode::Points) ? EPTPPreviewMode::Surface : EPTPPreviewMode::Points;
    // Re-run construction script to refresh preview
    RerunConstructionScripts();
}
