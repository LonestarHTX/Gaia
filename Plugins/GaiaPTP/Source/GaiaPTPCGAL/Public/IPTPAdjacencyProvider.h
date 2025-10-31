#pragma once

#include "CoreMinimal.h"

struct FPTPAdjacency
{
    TArray<TArray<int32>> Neighbors; // per-vertex neighbor indices
    TArray<FIntVector> Triangles;    // index triplets
};

class IPTPAdjacencyProvider
{
public:
    virtual ~IPTPAdjacencyProvider() = default;

    virtual bool Build(const TArray<FVector>& Points, FPTPAdjacency& OutAdj, FString& OutError) = 0;
};

TSharedPtr<IPTPAdjacencyProvider> CreateCGALAdjacencyProvider();

