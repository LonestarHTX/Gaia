#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PTPGameMode.generated.h"

/**
 * Game mode that spawns the PTPOrbitCamera as the default pawn.
 * Set this as your level's GameMode to automatically get the orbit camera.
 *
 * Note: This GameMode does NOT auto-spawn planets. Place a PTPPlanetActor
 * manually in your level. The orbit camera will find it automatically.
 */
UCLASS()
class GAIAPTP_API APTPGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APTPGameMode();

protected:
	virtual void BeginPlay() override;
};
