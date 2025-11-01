#pragma once

#include "CoreMinimal.h"

struct GAIAPTPCGAL_API FPTPAdjacency
{
    TArray<TArray<int32>> Neighbors; // per-vertex neighbor indices
    TArray<FIntVector> Triangles;    // index triplets
};

class GAIAPTPCGAL_API IPTPAdjacencyProvider
{
public:
    virtual ~IPTPAdjacencyProvider() = default;

    virtual bool Build(const TArray<FVector>& Points, FPTPAdjacency& OutAdj, FString& OutError) = 0;
};

GAIAPTPCGAL_API TSharedPtr<IPTPAdjacencyProvider> CreateCGALAdjacencyProvider();
