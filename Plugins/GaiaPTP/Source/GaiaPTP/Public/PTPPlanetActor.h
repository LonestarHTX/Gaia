#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PTPPlanetActor.generated.h"

class URealtimeMeshComponent;
class UPTPPlanetComponent;

UENUM(BlueprintType)
enum class EPTPPreviewMode : uint8
{
    Points   UMETA(DisplayName = "Points"),
    Surface  UMETA(DisplayName = "Surface")
};

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

    // Preview mode selector for editor visualization
    UPROPERTY(EditAnywhere, Category="PTP|Preview")
    EPTPPreviewMode PreviewMode = EPTPPreviewMode::Points;

    // Build/refresh CGAL adjacency (triangles & neighbors). Exposed as an editor button.
    UFUNCTION(CallInEditor, Category="PTP|Preview")
    void BuildAdjacency();

    // Convenience: Toggle between Points and Surface preview in-editor.
    UFUNCTION(CallInEditor, Category="PTP|Preview")
    void TogglePreviewMode();

    virtual void OnConstruction(const FTransform& Transform) override;
};
