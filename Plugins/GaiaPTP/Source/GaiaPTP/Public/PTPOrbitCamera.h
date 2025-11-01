#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PTPOrbitCamera.generated.h"

/**
 * Spore-style orbital camera for viewing procedural planets.
 *
 * Features:
 * - Smooth spherical orbit around a target point
 * - Click-drag to rotate, scroll to zoom
 * - Damped interpolation for polished feel
 * - Auto-targets PTPPlanetActor in the level
 */
UCLASS()
class GAIAPTP_API APTPOrbitCamera : public APawn
{
	GENERATED_BODY()

public:
	APTPOrbitCamera();

protected:
	// Camera component - what we see through
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* Camera;

	// Root scene component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USceneComponent* SceneRoot;

	// === Orbit Target ===

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	FVector TargetLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	bool bAutoTargetPlanet;

	// === Spherical Coordinates (Current - Interpolated) ===

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit|State")
	float CurrentAzimuth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit|State")
	float CurrentElevation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit|State")
	float CurrentDistance;

	// === Spherical Coordinates (Target - User Input) ===

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|State")
	float TargetAzimuth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|State")
	float TargetElevation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|State")
	float TargetDistance;

	// === Constraints ===

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Constraints")
	float MinDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Constraints")
	float MaxDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Constraints")
	float MinElevation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Constraints")
	float MaxElevation;

	// === Movement Settings ===

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Movement")
	float RotationSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Movement")
	float ZoomSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Movement")
	float Damping;

	// === Input State ===

	bool bIsOrbiting;
	FVector2D LastMousePosition;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	// Input handlers
	void StartOrbit();
	void StopOrbit();
	void OrbitCamera(float DeltaTime);
	void ZoomCamera(float AxisValue);
	void ToggleCursorMode();

	// Keyboard rotation
	void RotateHorizontal(float AxisValue);
	void RotateVertical(float AxisValue);

	// Keyboard zoom
	void KeyboardZoom(float AxisValue);

	// Update camera position from spherical coordinates
	void UpdateCameraPosition();

	// Find the planet actor if auto-targeting
	void FindPlanetTarget();
};
