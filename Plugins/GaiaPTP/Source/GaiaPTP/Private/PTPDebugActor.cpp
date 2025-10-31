#include "PTPDebugActor.h"
#include "RealtimeMeshComponent.h"

APTPDebugActor::APTPDebugActor()
{
    PrimaryActorTick.bCanEverTick = false;

    RealtimeMesh = CreateDefaultSubobject<URealtimeMeshComponent>(TEXT("RealtimeMesh"));
    SetRootComponent(RealtimeMesh);
}

