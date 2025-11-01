#include "PTPGameMode.h"
#include "PTPOrbitCamera.h"

APTPGameMode::APTPGameMode()
{
	// Set the default pawn to our orbit camera
	// When a player spawns in this game mode, they'll get the orbit camera
	DefaultPawnClass = APTPOrbitCamera::StaticClass();
}

void APTPGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Note: Auto-spawn removed - place PTPPlanetActor manually in your level
	// The orbit camera will find it automatically or use origin as fallback
}
