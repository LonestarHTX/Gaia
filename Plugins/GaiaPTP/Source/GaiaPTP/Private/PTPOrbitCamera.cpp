#include "PTPOrbitCamera.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "PTPPlanetActor.h"
#include "Kismet/GameplayStatics.h"

APTPOrbitCamera::APTPOrbitCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create components
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SceneRoot);

	// Initial spherical coordinates - nice default viewing angle
	CurrentAzimuth = TargetAzimuth = 45.0f;
	CurrentElevation = TargetElevation = 30.0f;
	CurrentDistance = TargetDistance = 2000000.0f;

	// Constraints (in Unreal units)
	// Planet radius: 6370km * 100 scale = 637,000 units
	MinDistance = 800000.0f;    // ~1.25x radius (close-up)
	MaxDistance = 15000000.0f;  // ~23x radius (far view)
	MinElevation = -80.0f;      // Don't go too low
	MaxElevation = 80.0f;       // Don't go too high

	// Movement settings - the "feel" parameters
	RotationSpeed = 0.2f;       // Degrees per pixel
	ZoomSpeed = 0.1f;           // Proportion per scroll (10%)
	Damping = 8.0f;             // Higher = more responsive

	// State
	bAutoTargetPlanet = true;
	TargetLocation = FVector::ZeroVector;
	bIsOrbiting = false;
}

void APTPOrbitCamera::BeginPlay()
{
	Super::BeginPlay();

	bool bFoundPlanet = false;

	if (bAutoTargetPlanet)
	{
		// Try finding the planet immediately
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APTPPlanetActor::StaticClass(), FoundActors);

		if (FoundActors.Num() > 0)
		{
			TargetLocation = FoundActors[0]->GetActorLocation();
			bFoundPlanet = true;
			UE_LOG(LogTemp, Log, TEXT("OrbitCamera: Auto-targeted %d planet(s), using first at %s"),
				FoundActors.Num(), *TargetLocation.ToString());
		}

		// If not found, retry after a short delay (planet might still be spawning)
		if (!bFoundPlanet)
		{
			UE_LOG(LogTemp, Warning, TEXT("OrbitCamera: No planet found, will retry..."));
			FTimerHandle RetryTimer;
			GetWorldTimerManager().SetTimer(RetryTimer, [this]()
			{
				UE_LOG(LogTemp, Warning, TEXT("OrbitCamera: Retrying planet search..."));
				FindPlanetTarget();
				UpdateCameraPosition();
			}, 0.5f, false); // Retry after 0.5 seconds
		}
	}

	// Log initial camera state for debugging
	UE_LOG(LogTemp, Log, TEXT("OrbitCamera: BeginPlay - Target: %s, Distance: %.0f, Azimuth: %.1f, Elevation: %.1f"),
		*TargetLocation.ToString(), CurrentDistance, CurrentAzimuth, CurrentElevation);

	UpdateCameraPosition();

	// Log where we actually ended up
	UE_LOG(LogTemp, Log, TEXT("OrbitCamera: Initial position: %s, looking at: %s"),
		*GetActorLocation().ToString(), *GetActorRotation().ToString());
}

void APTPOrbitCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsOrbiting)
	{
		OrbitCamera(DeltaTime);
	}

	// Handle azimuth wrapping for smooth interpolation
	// If target wraps from 359° to 1°, we don't want to interpolate the long way
	float AzimuthDelta = TargetAzimuth - CurrentAzimuth;
	if (AzimuthDelta > 180.0f)
	{
		CurrentAzimuth += 360.0f;
	}
	else if (AzimuthDelta < -180.0f)
	{
		CurrentAzimuth -= 360.0f;
	}

	// Smoothly interpolate current values toward target values
	CurrentAzimuth = FMath::FInterpTo(CurrentAzimuth, TargetAzimuth, DeltaTime, Damping);
	CurrentElevation = FMath::FInterpTo(CurrentElevation, TargetElevation, DeltaTime, Damping);
	CurrentDistance = FMath::FInterpTo(CurrentDistance, TargetDistance, DeltaTime, Damping);

	// Wrap current azimuth to 0-360 range
	while (CurrentAzimuth > 360.0f) CurrentAzimuth -= 360.0f;
	while (CurrentAzimuth < 0.0f) CurrentAzimuth += 360.0f;

	// Update camera position every frame
	UpdateCameraPosition();
}

void APTPOrbitCamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind mouse actions
	PlayerInputComponent->BindAction("OrbitCamera", IE_Pressed, this, &APTPOrbitCamera::StartOrbit);
	PlayerInputComponent->BindAction("OrbitCamera", IE_Released, this, &APTPOrbitCamera::StopOrbit);
	PlayerInputComponent->BindAxis("ZoomCamera", this, &APTPOrbitCamera::ZoomCamera);
	PlayerInputComponent->BindAction("ToggleCursor", IE_Pressed, this, &APTPOrbitCamera::ToggleCursorMode);

	// Bind keyboard rotation
	PlayerInputComponent->BindAxis("RotateHorizontal", this, &APTPOrbitCamera::RotateHorizontal);
	PlayerInputComponent->BindAxis("RotateVertical", this, &APTPOrbitCamera::RotateVertical);

	// Bind keyboard zoom
	PlayerInputComponent->BindAxis("KeyboardZoom", this, &APTPOrbitCamera::KeyboardZoom);
}

void APTPOrbitCamera::StartOrbit()
{
	bIsOrbiting = true;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		// Get viewport center and lock mouse there
		int32 ViewportSizeX, ViewportSizeY;
		PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
		LastMousePosition = FVector2D(ViewportSizeX / 2.0f, ViewportSizeY / 2.0f);

		PC->SetMouseLocation(LastMousePosition.X, LastMousePosition.Y);
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void APTPOrbitCamera::StopOrbit()
{
	bIsOrbiting = false;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		// Show cursor again when not orbiting
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeGameAndUI());
	}
}

void APTPOrbitCamera::OrbitCamera(float DeltaTime)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// Get current mouse position
	float MouseX, MouseY;
	PC->GetMousePosition(MouseX, MouseY);
	FVector2D CurrentMousePosition(MouseX, MouseY);

	// Calculate how far the mouse moved
	FVector2D MouseDelta = CurrentMousePosition - LastMousePosition;

	// Update target angles based on mouse movement
	// X movement = azimuth (horizontal spin)
	// Y movement = elevation (up/down angle)
	TargetAzimuth += MouseDelta.X * RotationSpeed;
	TargetElevation -= MouseDelta.Y * RotationSpeed; // Negative because screen Y is inverted

	// Clamp elevation to avoid gimbal lock at poles
	TargetElevation = FMath::Clamp(TargetElevation, MinElevation, MaxElevation);

	// Wrap azimuth to 0-360 range
	while (TargetAzimuth > 360.0f) TargetAzimuth -= 360.0f;
	while (TargetAzimuth < 0.0f) TargetAzimuth += 360.0f;

	// Reset mouse to center each frame (allows infinite rotation)
	int32 ViewportSizeX, ViewportSizeY;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
	LastMousePosition = FVector2D(ViewportSizeX / 2.0f, ViewportSizeY / 2.0f);
	PC->SetMouseLocation(LastMousePosition.X, LastMousePosition.Y);
}

void APTPOrbitCamera::ZoomCamera(float AxisValue)
{
	if (FMath::Abs(AxisValue) > 0.01f)
	{
		// Proportional zoom: ZoomSpeed % of current distance per scroll
		const float ProportionalSpeed = CurrentDistance * ZoomSpeed;
		TargetDistance -= AxisValue * ProportionalSpeed;
		TargetDistance = FMath::Clamp(TargetDistance, MinDistance, MaxDistance);
	}
}

void APTPOrbitCamera::UpdateCameraPosition()
{
	// Convert spherical coordinates to Cartesian
	const float AzimuthRad = FMath::DegreesToRadians(CurrentAzimuth);
	const float ElevationRad = FMath::DegreesToRadians(CurrentElevation);

	// Spherical to Cartesian conversion:
	// x = r * cos(elevation) * cos(azimuth)
	// y = r * cos(elevation) * sin(azimuth)
	// z = r * sin(elevation)
	const float HorizontalDistance = CurrentDistance * FMath::Cos(ElevationRad);
	const float X = HorizontalDistance * FMath::Cos(AzimuthRad);
	const float Y = HorizontalDistance * FMath::Sin(AzimuthRad);
	const float Z = CurrentDistance * FMath::Sin(ElevationRad);

	const FVector CameraOffset(X, Y, Z);
	const FVector NewCameraLocation = TargetLocation + CameraOffset;

	// Position the camera
	SetActorLocation(NewCameraLocation);

	// Make camera look at the target
	const FVector DirectionToTarget = (TargetLocation - NewCameraLocation).GetSafeNormal();
	const FRotator NewCameraRotation = DirectionToTarget.Rotation();
	SetActorRotation(NewCameraRotation);
}

void APTPOrbitCamera::FindPlanetTarget()
{
	// Find all PTPPlanetActors in the level
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APTPPlanetActor::StaticClass(), FoundActors);

	if (FoundActors.Num() > 0)
	{
		// Use the first planet we find
		TargetLocation = FoundActors[0]->GetActorLocation();
		UE_LOG(LogTemp, Log, TEXT("OrbitCamera: Auto-targeted %d planet(s), using first at %s"),
			FoundActors.Num(), *TargetLocation.ToString());
	}
	else
	{
		// No planet found, use world origin as fallback
		UE_LOG(LogTemp, Warning, TEXT("OrbitCamera: No PTPPlanetActor found in level, using origin (0,0,0)"));
		TargetLocation = FVector::ZeroVector;
	}
}

void APTPOrbitCamera::ToggleCursorMode()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// Toggle cursor visibility
	PC->bShowMouseCursor = !PC->bShowMouseCursor;

	if (PC->bShowMouseCursor)
	{
		PC->SetInputMode(FInputModeGameAndUI());
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void APTPOrbitCamera::RotateHorizontal(float AxisValue)
{
	if (FMath::Abs(AxisValue) > 0.01f)
	{
		// Keyboard rotation - scale up for smooth motion
		// RotationSpeed is degrees per pixel for mouse, multiply by 25 for keyboard
		TargetAzimuth += AxisValue * RotationSpeed * 25.0f;

		// Wrap azimuth to 0-360 range
		while (TargetAzimuth > 360.0f) TargetAzimuth -= 360.0f;
		while (TargetAzimuth < 0.0f) TargetAzimuth += 360.0f;
	}
}

void APTPOrbitCamera::RotateVertical(float AxisValue)
{
	if (FMath::Abs(AxisValue) > 0.01f)
	{
		// Keyboard rotation - scale up for smooth motion
		TargetElevation += AxisValue * RotationSpeed * 25.0f;

		// Clamp elevation to constraints
		TargetElevation = FMath::Clamp(TargetElevation, MinElevation, MaxElevation);
	}
}

void APTPOrbitCamera::KeyboardZoom(float AxisValue)
{
	if (FMath::Abs(AxisValue) > 0.01f)
	{
		// Keyboard zoom - use proportional speed like mouse wheel
		// Scale up for responsiveness (25% of current distance per second)
		const float ProportionalSpeed = CurrentDistance * 0.25f;
		TargetDistance -= AxisValue * ProportionalSpeed * GetWorld()->GetDeltaSeconds();
		TargetDistance = FMath::Clamp(TargetDistance, MinDistance, MaxDistance);
	}
}
