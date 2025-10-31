#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PTPPlanetActor.generated.h"

class URealtimeMeshComponent;
class UPTPPlanetComponent;

/** Actor that hosts the planet component and an RMC for visualization. */
UCLASS()
class GAIAPTP_API APTPPlanetActor : public AActor
{
    GENERATED_BODY()
public:
    APTPPlanetActor();

protected:
    UPROPERTY(VisibleAnywhere, Category="PTP")
    URealtimeMeshComponent* RealtimeMesh;

    UPROPERTY(VisibleAnywhere, Category="PTP")
    UPTPPlanetComponent* Planet;

    virtual void OnConstruction(const FTransform& Transform) override;
};

