#pragma once

#include "CoreMinimal.h"
#include "TectonicTypes.generated.h"

UENUM(BlueprintType)
enum class ECrustType : uint8
{
    Oceanic     UMETA(DisplayName="Oceanic"),
    Continental UMETA(DisplayName="Continental")
};

UENUM(BlueprintType)
enum class EOrogenyType : uint8
{
    None        UMETA(DisplayName="None"),
    Andean      UMETA(DisplayName="Andean"),       // Subduction orogeny (volcanic)
    Himalayan   UMETA(DisplayName="Himalayan")     // Continental collision
};

