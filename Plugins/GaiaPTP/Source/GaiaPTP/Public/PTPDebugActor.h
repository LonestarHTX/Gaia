#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PTPDebugActor.generated.h"

class URealtimeMeshComponent;

/** Minimal debug actor that will host RealtimeMesh visualizations. */
UCLASS()
class GAIAPTP_API APTPDebugActor : public AActor
{
    GENERATED_BODY()

public:
    APTPDebugActor();

protected:
    UPROPERTY(VisibleAnywhere, Category="PTP")
    URealtimeMeshComponent* RealtimeMesh;
};

