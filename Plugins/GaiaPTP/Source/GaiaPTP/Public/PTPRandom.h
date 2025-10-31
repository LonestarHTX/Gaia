#pragma once

#include "CoreMinimal.h"

struct FPTPRandom
{
    FRandomStream Stream;

    explicit FPTPRandom(int32 Seed = 12345)
        : Stream(Seed)
    {}

    void Initialize(int32 Seed) { Stream.Initialize(Seed); }

    int32 RandRange(int32 Min, int32 Max) { return Stream.RandRange(Min, Max); }
    float FRand() { return Stream.FRand(); }
    FVector VRand() { return Stream.VRand(); }
};

