# Building a Spore-Style Orbital Camera
## A Coding Adventure in Unreal Engine 5

*Written in the style of Sebastian Lague's coding adventures*

---

## Introduction: The Joy of Orbiting Spheres

So, we've just finished building this beautiful tectonic planet system with 40 colorful plates, and... we can't really look at it properly. The default editor camera is fine for placing actors, but when you want to examine a *sphere* - something that begs to be rotated and viewed from all angles - you need something better.

If you've ever played Spore (and if you haven't, you should - it's a masterpiece of procedural generation), you'll remember how *good* the planet camera felt. You'd click and drag, and the planet would smoothly rotate beneath your cursor. Zoom in, zoom out, always focused on that spherical world. It felt... *right*.

Let me show you how we can build that same feeling for our tectonic planets.

### What Makes It Feel Good?

Before we write any code, let's think about what we're actually building. The Spore camera has a few key qualities:

1. **It orbits around a point** - not the camera moving through space, but rotating *around* the planet
2. **Smooth, damped motion** - when you stop dragging, it doesn't just snap to a halt
3. **Distance constraints** - you can't zoom through the planet or infinitely far away
4. **The horizon stays level** - no weird rolling or tilting
5. **Responsive but not twitchy** - there's a subtle weight to the movement

That last one is the secret sauce. Too fast and it feels like you're whipping a camera around on a string. Too slow and it feels sluggish and unresponsive. There's a sweet spot, and we'll find it together.

---

## Chapter 1: Spherical Coordinates (The Foundation)

### Understanding the Math

Here's the thing about cameras orbiting spheres - Cartesian coordinates (X, Y, Z) are actually pretty awkward for this. What we *really* want is **spherical coordinates**:

- **Azimuth (θ)** - rotation around the vertical axis (think: spinning the planet left/right)
- **Elevation (φ)** - angle up/down from the horizon
- **Distance (r)** - how far from the planet center

Wait, let me draw this out in my head...

```
         Y (Up)
         |
         |
         * Camera
        /|\
       / | \
      /  |  \
     /   |r  \
    /    |    \
   /_____|_____\
  /  φ   |      \
 /-------|-------\
X        |        Z
         |
    Planet Center

Azimuth θ: rotation around Y axis (horizontal spinning)
Elevation φ: angle from horizon plane
Distance r: straight-line distance from center
```

The conversion from spherical to Cartesian is:
```
x = r * cos(φ) * cos(θ)
y = r * sin(φ)
z = r * cos(φ) * sin(θ)
```

Actually, wait - let me verify that. If φ = 0 (horizon level), then sin(φ) = 0, so y = 0. That's correct - we're on the horizontal plane. And if φ = 90° (directly above), then cos(φ) = 0, so x and z become 0, and y = r. Yes, that checks out. Good.

### Why Spherical Coordinates?

When the player drags their mouse left, we don't want to move the camera *left* - we want to increase the azimuth angle. When they drag up, we want to increase elevation. When they scroll the mouse wheel, we want to change distance.

Each input maps to *one* parameter. That's the elegance of spherical coordinates for orbital cameras.

---

## Chapter 2: The Camera Pawn (Setting Up The Structure)

### Creating the Class

Let's start building. We'll create a new Pawn class called `APTPOrbitCamera`. Why a Pawn and not just a Camera component? Because we want the player to *possess* this camera, to control it directly with their inputs.

**File: `GaiaPTP/Public/PTPOrbitCamera.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PTPOrbitCamera.generated.h"

UCLASS()
class GAIAPTP_API APTPOrbitCamera : public APawn
{
    GENERATED_BODY()

public:
    APTPOrbitCamera();

protected:
    // The camera component - this is what we actually see through
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class UCameraComponent* Camera;

    // The root component - we'll use a simple scene component as anchor
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class USceneComponent* SceneRoot;

    // What are we orbiting around?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
    FVector TargetLocation;

    // Auto-find PTPPlanetActor in the level and orbit around it?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
    bool bAutoTargetPlanet;

    // Spherical coordinates (the heart of our system)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
    float Azimuth;          // Horizontal rotation (degrees)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
    float Elevation;        // Vertical angle (degrees)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
    float Distance;         // How far from target

    // Constraints
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Constraints")
    float MinDistance;      // Can't get closer than this

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Constraints")
    float MaxDistance;      // Can't get farther than this

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Constraints")
    float MinElevation;     // Don't go below the ground

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Constraints")
    float MaxElevation;     // Don't flip over the top

    // Movement settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Movement")
    float RotationSpeed;    // How fast does mouse drag rotate?

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Movement")
    float ZoomSpeed;        // How fast does scroll wheel zoom?

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Movement")
    float Damping;          // Smoothing factor (0 = instant, 1 = never moves)

    // Input state
    bool bIsOrbiting;       // Currently dragging?
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

    // Update camera position from spherical coordinates
    void UpdateCameraPosition();

    // Find the planet actor if auto-targeting
    void FindPlanetTarget();
};
```

Okay, so that's our foundation. Looking at this, I'm noticing we have three main responsibilities:

1. **Track spherical coordinates** (Azimuth, Elevation, Distance)
2. **Handle input** (mouse drag, scroll wheel)
3. **Convert to world position** and update the camera

Let me think about the constraints for a moment. For elevation, we probably want to clamp between -85° and +85° (not quite -90° to +90°, because weird gimbal-lock stuff happens at the poles). And for distance... well, that depends on the planet size. If our planet radius is 6,370 km with a visualization scale of 100, the actual rendered size is 637,000 units. So maybe MinDistance = 1,000,000 (close-up view) and MaxDistance = 10,000,000 (far view)?

Actually, let's make those configurable and set sensible defaults in the constructor.

---

## Chapter 3: The Constructor (Setting Sensible Defaults)

**File: `GaiaPTP/Private/PTPOrbitCamera.cpp`**

```cpp
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

    // Default spherical coordinates - start looking at the planet from a nice angle
    Azimuth = 45.0f;        // 45° around
    Elevation = 30.0f;      // 30° above horizon (good default view)
    Distance = 2000000.0f;  // 2 million units (nice medium distance)

    // Constraints (these are in Unreal units)
    // Remember: planet radius is 6370km * scale 100 = 637,000 units
    MinDistance = 800000.0f;    // ~1.25x planet radius (close-up)
    MaxDistance = 15000000.0f;  // ~23x planet radius (very far)
    MinElevation = -80.0f;      // Don't go too low
    MaxElevation = 80.0f;       // Don't go too high

    // Movement settings - these are the "feel" knobs we'll tune
    RotationSpeed = 0.2f;       // Degrees per pixel of mouse movement
    ZoomSpeed = 100000.0f;      // Units per mouse wheel click
    Damping = 0.0f;             // Start with no damping, we'll add it later

    // State
    bAutoTargetPlanet = true;   // Find the planet automatically
    TargetLocation = FVector::ZeroVector;
    bIsOrbiting = false;
}
```

Hmm, looking at those numbers, I'm wondering if they're actually right. Let's double-check the math:

- Planet radius in code: 6,370 km
- Visualization scale: 100
- Rendered radius: 6,370 × 100 = 637,000 units

So MinDistance = 800,000 means we're 1.25 radii away from the center, or 0.25 radii above the surface. That feels about right for a close-up view.

MaxDistance = 15,000,000 means we're 23.5 radii away - that's far enough to see the whole planet as a sphere with good context.

Okay, I'm confident in those numbers. Let's continue.

---

## Chapter 4: Finding Our Target (Auto-Focus on the Planet)

When the camera starts up, we want it to automatically find the PTPPlanetActor in the level and orbit around it. This is one of those "small things that make it feel polished" features.

```cpp
void APTPOrbitCamera::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoTargetPlanet)
    {
        FindPlanetTarget();
    }

    // Set initial camera position
    UpdateCameraPosition();
}

void APTPOrbitCamera::FindPlanetTarget()
{
    // Find the first PTPPlanetActor in the level
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APTPPlanetActor::StaticClass(), FoundActors);

    if (FoundActors.Num() > 0)
    {
        // Use the first planet we find
        AActor* PlanetActor = FoundActors[0];
        TargetLocation = PlanetActor->GetActorLocation();

        UE_LOG(LogTemp, Log, TEXT("OrbitCamera: Auto-targeted planet at %s"), *TargetLocation.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("OrbitCamera: No PTPPlanetActor found in level, using origin"));
        TargetLocation = FVector::ZeroVector;
    }
}
```

Simple and effective. If there's a planet, we orbit it. If not, we orbit the world origin. Good fallback behavior.

---

## Chapter 5: The Heart of It All (Spherical to Cartesian Conversion)

This is the function that gets called every frame to position the camera. It's where the magic happens - converting our nice clean spherical coordinates into a world position.

```cpp
void APTPOrbitCamera::UpdateCameraPosition()
{
    // Convert degrees to radians (because math functions expect radians)
    const float AzimuthRad = FMath::DegreesToRadians(Azimuth);
    const float ElevationRad = FMath::DegreesToRadians(Elevation);

    // Spherical to Cartesian conversion
    // x = r * cos(elevation) * cos(azimuth)
    // y = r * cos(elevation) * sin(azimuth)
    // z = r * sin(elevation)

    // Wait, let me think about Unreal's coordinate system for a second...
    // Unreal uses: X = forward, Y = right, Z = up
    // So if we want azimuth to spin horizontally and elevation to go up/down:

    const float HorizontalDistance = Distance * FMath::Cos(ElevationRad);
    const float X = HorizontalDistance * FMath::Cos(AzimuthRad);
    const float Y = HorizontalDistance * FMath::Sin(AzimuthRad);
    const float Z = Distance * FMath::Sin(ElevationRad);

    const FVector CameraOffset(X, Y, Z);
    const FVector NewCameraLocation = TargetLocation + CameraOffset;

    // Set the pawn's location (which will move the camera component with it)
    SetActorLocation(NewCameraLocation);

    // Make the camera look at the target
    const FVector DirectionToTarget = (TargetLocation - NewCameraLocation).GetSafeNormal();
    const FRotator NewCameraRotation = DirectionToTarget.Rotation();
    SetActorRotation(NewCameraRotation);
}
```

Let me trace through this with an example to make sure it's right:

**Example: Azimuth = 0°, Elevation = 0°, Distance = 1000**
- AzimuthRad = 0, ElevationRad = 0
- HorizontalDistance = 1000 * cos(0) = 1000 * 1 = 1000
- X = 1000 * cos(0) = 1000 * 1 = 1000
- Y = 1000 * sin(0) = 1000 * 0 = 0
- Z = 1000 * sin(0) = 1000 * 0 = 0
- Result: (1000, 0, 0) - directly in front on the X axis

**Example: Azimuth = 90°, Elevation = 0°, Distance = 1000**
- AzimuthRad = π/2, ElevationRad = 0
- HorizontalDistance = 1000
- X = 1000 * cos(π/2) = 1000 * 0 = 0
- Y = 1000 * sin(π/2) = 1000 * 1 = 1000
- Z = 0
- Result: (0, 1000, 0) - directly to the right on the Y axis

**Example: Azimuth = 0°, Elevation = 90°, Distance = 1000**
- HorizontalDistance = 1000 * cos(π/2) = 0
- X = 0, Y = 0
- Z = 1000 * sin(π/2) = 1000
- Result: (0, 0, 1000) - directly above on the Z axis

Perfect! The math checks out. When we increase azimuth, we rotate horizontally. When we increase elevation, we go up. Distance controls how far away we are. Exactly what we want.

---

## Chapter 6: Handling Input (Making It Interactive)

Now for the fun part - making it actually respond to the player's input. We need to handle:
1. Mouse drag to orbit
2. Mouse wheel to zoom

```cpp
void APTPOrbitCamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Bind mouse button for orbiting
    PlayerInputComponent->BindAction("OrbitCamera", IE_Pressed, this, &APTPOrbitCamera::StartOrbit);
    PlayerInputComponent->BindAction("OrbitCamera", IE_Released, this, &APTPOrbitCamera::StopOrbit);

    // Bind mouse wheel for zooming
    PlayerInputComponent->BindAxis("ZoomCamera", this, &APTPOrbitCamera::ZoomCamera);
}

void APTPOrbitCamera::StartOrbit()
{
    bIsOrbiting = true;

    // Get the current mouse position so we can track deltas
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC)
    {
        float MouseX, MouseY;
        PC->GetMousePosition(MouseX, MouseY);
        LastMousePosition = FVector2D(MouseX, MouseY);
    }
}

void APTPOrbitCamera::StopOrbit()
{
    bIsOrbiting = false;
}

void APTPOrbitCamera::ZoomCamera(float AxisValue)
{
    if (FMath::Abs(AxisValue) > 0.01f)
    {
        // AxisValue is typically 1.0 or -1.0 for mouse wheel
        Distance -= AxisValue * ZoomSpeed;

        // Clamp to constraints
        Distance = FMath::Clamp(Distance, MinDistance, MaxDistance);

        UpdateCameraPosition();
    }
}
```

Wait, I just realized something. For the orbiting, I'm storing the mouse position in `StartOrbit()`, but I haven't actually written the code that *uses* that to rotate the camera. Let me add that to the Tick function:

```cpp
void APTPOrbitCamera::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsOrbiting)
    {
        OrbitCamera(DeltaTime);
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

    // Update spherical coordinates based on mouse movement
    // X movement = azimuth (horizontal rotation)
    // Y movement = elevation (vertical rotation)
    Azimuth += MouseDelta.X * RotationSpeed;
    Elevation -= MouseDelta.Y * RotationSpeed; // Negative because screen Y is inverted

    // Clamp elevation to avoid gimbal lock at poles
    Elevation = FMath::Clamp(Elevation, MinElevation, MaxElevation);

    // Wrap azimuth to 0-360 range (not strictly necessary, but keeps numbers clean)
    while (Azimuth > 360.0f) Azimuth -= 360.0f;
    while (Azimuth < 0.0f) Azimuth += 360.0f;

    // Update the camera position
    UpdateCameraPosition();

    // Store current position for next frame's delta calculation
    LastMousePosition = CurrentMousePosition;
}
```

Okay, this is starting to feel right. The mouse delta gets multiplied by `RotationSpeed` (which we set to 0.2 degrees per pixel), and that updates our azimuth and elevation.

But wait... there's a problem here I need to think about.

---

## Chapter 7: The Mouse Cursor Problem (A Quick Detour)

Actually, there's an issue with what I just wrote. When you're dragging the mouse to orbit the camera, the mouse cursor is going to move across the screen. Eventually, it'll hit the edge of the window and you can't drag any further.

This is annoying! In a real orbital camera (like in Blender or Unity's Scene view), the cursor either:
1. Gets hidden and locked to the center, or
2. Wraps around the screen edges

Let's do option 1 - it's cleaner:

```cpp
void APTPOrbitCamera::StartOrbit()
{
    bIsOrbiting = true;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC)
    {
        // Get viewport center
        int32 ViewportSizeX, ViewportSizeY;
        PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
        LastMousePosition = FVector2D(ViewportSizeX / 2, ViewportSizeY / 2);

        // Lock cursor to center and hide it
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
        // Show cursor again
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

    // Calculate mouse delta
    FVector2D MouseDelta = CurrentMousePosition - LastMousePosition;

    // Update angles
    Azimuth += MouseDelta.X * RotationSpeed;
    Elevation -= MouseDelta.Y * RotationSpeed;

    // Clamp elevation
    Elevation = FMath::Clamp(Elevation, MinElevation, MaxElevation);

    // Wrap azimuth
    while (Azimuth > 360.0f) Azimuth -= 360.0f;
    while (Azimuth < 0.0f) Azimuth += 360.0f;

    UpdateCameraPosition();

    // Reset mouse to center each frame so it never hits edges
    int32 ViewportSizeX, ViewportSizeY;
    PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
    LastMousePosition = FVector2D(ViewportSizeX / 2, ViewportSizeY / 2);
    PC->SetMouseLocation(LastMousePosition.X, LastMousePosition.Y);
}
```

Much better! Now when you click to orbit, the cursor disappears and gets locked to the center of the screen. Every frame, we read the delta from center, apply it to the camera angles, then reset the mouse back to center. Infinite orbiting!

---

## Chapter 8: Input Mapping (Connecting to Unreal's Input System)

Now we need to actually tell Unreal what "OrbitCamera" and "ZoomCamera" mean in terms of actual keyboard/mouse inputs. This goes in the project settings, but let me show you what the config should look like:

**File: `Config/DefaultInput.ini`**

Add these lines:

```ini
[/Script/Engine.InputSettings]
+ActionMappings=(ActionName="OrbitCamera",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=LeftMouseButton)
+ActionMappings=(ActionName="OrbitCamera",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=RightMouseButton)
+AxisMappings=(AxisName="ZoomCamera",Scale=1.000000,Key=MouseWheelAxis)
```

This maps:
- Left mouse button → OrbitCamera action
- Right mouse button → OrbitCamera action (some people prefer right-click)
- Mouse wheel → ZoomCamera axis

Perfect. Now the inputs will actually work.

---

## Chapter 9: Testing Our Camera (The Moment of Truth)

Okay, let's compile this and test it! Here's what we need to do:

1. Add the class to the plugin's module
2. Build the project
3. Place an APTPOrbitCamera in the level
4. Set it as the player start's possessed pawn
5. Hit Play and try it out

Actually, wait - let me think about step 4 for a moment. In the editor, we don't usually have a "player start" automatically possessing a pawn. We need to either:
- Create a custom GameMode that spawns our camera, OR
- Use a PlayerStart and set its AutoPossess property, OR
- Just manually possess it in the level blueprint for testing

Let's do the GameMode approach - it's cleaner for a permanent solution:

**File: `GaiaPTP/Public/PTPGameMode.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PTPGameMode.generated.h"

UCLASS()
class GAIAPTP_API APTPGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    APTPGameMode();
};
```

**File: `GaiaPTP/Private/PTPGameMode.cpp`**

```cpp
#include "PTPGameMode.h"
#include "PTPOrbitCamera.h"

APTPGameMode::APTPGameMode()
{
    // Set the default pawn to our orbit camera
    DefaultPawnClass = APTPOrbitCamera::StaticClass();
}
```

Now, in the project settings (or in the level's World Settings), we can set this as the GameMode, and players will automatically start with our orbit camera.

---

## Chapter 10: Adding Smoothness (The Secret Sauce)

Okay, so at this point, the camera works. You can orbit, you can zoom. But it probably feels a bit... *harsh*. When you stop moving the mouse, the camera stops instantly. When you scroll the zoom wheel, it jumps in discrete steps.

This is where **damping** comes in - the secret ingredient that makes motion feel smooth and weighted.

The idea is simple: instead of instantly setting the camera position, we *interpolate* toward the target position over time. In math terms:

```
CurrentValue = Lerp(CurrentValue, TargetValue, DampingFactor * DeltaTime)
```

Let's add this. We'll need to track "target" values separately from "current" values:

**Updated class variables:**

```cpp
// Current spherical coordinates (smoothly interpolated)
float CurrentAzimuth;
float CurrentElevation;
float CurrentDistance;

// Target spherical coordinates (what we're moving toward)
float TargetAzimuth;
float TargetElevation;
float TargetDistance;
```

**Updated constructor:**

```cpp
APTPOrbitCamera::APTPOrbitCamera()
{
    // ... existing code ...

    // Initialize both current and target to the same values
    CurrentAzimuth = TargetAzimuth = 45.0f;
    CurrentElevation = TargetElevation = 30.0f;
    CurrentDistance = TargetDistance = 2000000.0f;

    // Set damping to something reasonable
    Damping = 8.0f;  // Higher = more responsive, lower = more sluggish
}
```

**Updated OrbitCamera:**

```cpp
void APTPOrbitCamera::OrbitCamera(float DeltaTime)
{
    // ... existing mouse position code ...

    // Update TARGET angles (not current)
    TargetAzimuth += MouseDelta.X * RotationSpeed;
    TargetElevation -= MouseDelta.Y * RotationSpeed;

    // Clamp target elevation
    TargetElevation = FMath::Clamp(TargetElevation, MinElevation, MaxElevation);

    // Wrap target azimuth
    while (TargetAzimuth > 360.0f) TargetAzimuth -= 360.0f;
    while (TargetAzimuth < 0.0f) TargetAzimuth += 360.0f;

    // ... existing mouse reset code ...
}
```

**Updated ZoomCamera:**

```cpp
void APTPOrbitCamera::ZoomCamera(float AxisValue)
{
    if (FMath::Abs(AxisValue) > 0.01f)
    {
        // Update TARGET distance (not current)
        TargetDistance -= AxisValue * ZoomSpeed;
        TargetDistance = FMath::Clamp(TargetDistance, MinDistance, MaxDistance);
    }
}
```

**Updated Tick:**

```cpp
void APTPOrbitCamera::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsOrbiting)
    {
        OrbitCamera(DeltaTime);
    }

    // Smoothly interpolate current values toward target values
    const float InterpSpeed = Damping;
    CurrentAzimuth = FMath::FInterpTo(CurrentAzimuth, TargetAzimuth, DeltaTime, InterpSpeed);
    CurrentElevation = FMath::FInterpTo(CurrentElevation, TargetElevation, DeltaTime, InterpSpeed);
    CurrentDistance = FMath::FInterpTo(CurrentDistance, TargetDistance, DeltaTime, InterpSpeed);

    // Update camera position using CURRENT (interpolated) values
    UpdateCameraPosition();
}
```

**Updated UpdateCameraPosition:**

```cpp
void APTPOrbitCamera::UpdateCameraPosition()
{
    // Use CURRENT (interpolated) values, not target values
    const float AzimuthRad = FMath::DegreesToRadians(CurrentAzimuth);
    const float ElevationRad = FMath::DegreesToRadians(CurrentElevation);

    const float HorizontalDistance = CurrentDistance * FMath::Cos(ElevationRad);
    const float X = HorizontalDistance * FMath::Cos(AzimuthRad);
    const float Y = HorizontalDistance * FMath::Sin(AzimuthRad);
    const float Z = CurrentDistance * FMath::Sin(ElevationRad);

    // ... rest of the function stays the same ...
}
```

Now when you move the mouse, the camera doesn't instantly snap to the new position - it smoothly glides there. The `Damping` value controls how quickly it catches up. Higher values make it more responsive, lower values make it more floaty.

Try different values and find what feels good to you! I like around 8.0 - responsive enough to feel direct, but smooth enough to feel polished.

---

## Chapter 11: Edge Cases and Polish

Let's think about what could go wrong:

### Problem 1: Azimuth Wrapping

When we wrap the target azimuth from 359° to 1°, the current azimuth might try to interpolate the "long way around" (359 → 180 → 90 → 1) instead of the short way (359 → 360 → 0 → 1).

Fix: Detect wrapping and adjust the current value:

```cpp
// In Tick(), before interpolation:
float AzimuthDelta = TargetAzimuth - CurrentAzimuth;
if (AzimuthDelta > 180.0f) CurrentAzimuth += 360.0f;
else if (AzimuthDelta < -180.0f) CurrentAzimuth -= 360.0f;

CurrentAzimuth = FMath::FInterpTo(CurrentAzimuth, TargetAzimuth, DeltaTime, InterpSpeed);

// Wrap current back to 0-360 range
while (CurrentAzimuth > 360.0f) CurrentAzimuth -= 360.0f;
while (CurrentAzimuth < 0.0f) CurrentAzimuth += 360.0f;
```

### Problem 2: Zoom Speed Feels Wrong at Different Distances

When you're far away, scrolling one notch moves you a fixed distance. But when you're close, that same fixed distance is a huge percentage of your distance! It feels inconsistent.

Fix: Make zoom speed proportional to current distance:

```cpp
void APTPOrbitCamera::ZoomCamera(float AxisValue)
{
    if (FMath::Abs(AxisValue) > 0.01f)
    {
        // Make zoom speed proportional to current distance (10% per scroll)
        const float ProportionalSpeed = CurrentDistance * 0.1f;
        TargetDistance -= AxisValue * ProportionalSpeed;
        TargetDistance = FMath::Clamp(TargetDistance, MinDistance, MaxDistance);
    }
}
```

Much better! Now zooming feels consistent whether you're close or far.

### Problem 3: Can't Use Editor UI While In Game

When we set `FInputModeGameOnly()`, we can't click on editor UI anymore. Let's make it so you can toggle between game mode and UI mode with the Escape key:

```cpp
// Add to SetupPlayerInputComponent:
PlayerInputComponent->BindAction("ToggleCursor", IE_Pressed, this, &APTPOrbitCamera::ToggleCursorMode);

// Add new function:
void APTPOrbitCamera::ToggleCursorMode()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

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
```

And in `Config/DefaultInput.ini`:

```ini
+ActionMappings=(ActionName="ToggleCursor",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=Escape)
```

---

## Chapter 12: Optional Features (Going Further)

Now we have a solid, working Spore-style camera. But there are some extra features we could add:

### Feature 1: Double-Click to Focus

Wouldn't it be nice if you could double-click on the planet to smoothly zoom to that point? This would require raycasting to find where you clicked, then setting that as the new target location.

### Feature 2: Keyboard Movement

Some people like WASD to rotate instead of mouse drag. Easy to add:

```cpp
// In SetupPlayerInputComponent:
PlayerInputComponent->BindAxis("OrbitRight", this, &APTPOrbitCamera::OrbitRight);
PlayerInputComponent->BindAxis("OrbitUp", this, &APTPOrbitCamera::OrbitUp);

// New functions:
void APTPOrbitCamera::OrbitRight(float AxisValue)
{
    TargetAzimuth += AxisValue * RotationSpeed * 50.0f; // Scale up for keyboard
}

void APTPOrbitCamera::OrbitUp(float AxisValue)
{
    TargetElevation += AxisValue * RotationSpeed * 50.0f;
    TargetElevation = FMath::Clamp(TargetElevation, MinElevation, MaxElevation);
}
```

```ini
+AxisMappings=(AxisName="OrbitRight",Scale=1.000000,Key=D)
+AxisMappings=(AxisName="OrbitRight",Scale=-1.000000,Key=A)
+AxisMappings=(AxisName="OrbitUp",Scale=1.000000,Key=W)
+AxisMappings=(AxisName="OrbitUp",Scale=-1.000000,Key=S)
```

### Feature 3: Momentum

Real Spore camera has momentum - when you let go, it doesn't stop instantly, it coasts to a stop. This requires tracking velocity:

```cpp
// Add to class:
float AzimuthVelocity;
float ElevationVelocity;
float MomentumDecay; // How quickly velocity decays (0-1, lower = more coasting)

// In OrbitCamera:
float DeltaAzimuth = MouseDelta.X * RotationSpeed;
float DeltaElevation = MouseDelta.Y * RotationSpeed;

// Instead of directly setting target, add to velocity
AzimuthVelocity = DeltaAzimuth / DeltaTime; // pixels/second
ElevationVelocity = -DeltaElevation / DeltaTime;

// In Tick:
if (!bIsOrbiting)
{
    // Apply momentum decay when not actively dragging
    AzimuthVelocity *= FMath::Pow(MomentumDecay, DeltaTime);
    ElevationVelocity *= FMath::Pow(MomentumDecay, DeltaTime);
}

// Always apply velocity to target
TargetAzimuth += AzimuthVelocity * DeltaTime;
TargetElevation += ElevationVelocity * DeltaTime;
```

This is getting complex, but it feels *amazing* when tuned right.

---

## Chapter 13: Final Code Checkpoint

Alright, let me consolidate everything we've built into the complete, working code:

**File: `GaiaPTP/Public/PTPOrbitCamera.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PTPOrbitCamera.generated.h"

UCLASS()
class GAIAPTP_API APTPOrbitCamera : public APawn
{
    GENERATED_BODY()

public:
    APTPOrbitCamera();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class UCameraComponent* Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class USceneComponent* SceneRoot;

    // Orbit target
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
    FVector TargetLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
    bool bAutoTargetPlanet;

    // Spherical coordinates (current - interpolated)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit|State")
    float CurrentAzimuth;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit|State")
    float CurrentElevation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit|State")
    float CurrentDistance;

    // Spherical coordinates (target - user input)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|State")
    float TargetAzimuth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|State")
    float TargetElevation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|State")
    float TargetDistance;

    // Constraints
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Constraints")
    float MinDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Constraints")
    float MaxDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Constraints")
    float MinElevation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Constraints")
    float MaxElevation;

    // Movement settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Movement")
    float RotationSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Movement")
    float ZoomSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Movement")
    float Damping;

    // Input state
    bool bIsOrbiting;
    FVector2D LastMousePosition;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
    void StartOrbit();
    void StopOrbit();
    void OrbitCamera(float DeltaTime);
    void ZoomCamera(float AxisValue);
    void UpdateCameraPosition();
    void FindPlanetTarget();
    void ToggleCursorMode();
};
```

**File: `GaiaPTP/Private/PTPOrbitCamera.cpp`**

```cpp
#include "PTPOrbitCamera.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "PTPPlanetActor.h"
#include "Kismet/GameplayStatics.h"

APTPOrbitCamera::APTPOrbitCamera()
{
    PrimaryActorTick.bCanEverTick = true;

    // Components
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SceneRoot);

    // Initial values
    CurrentAzimuth = TargetAzimuth = 45.0f;
    CurrentElevation = TargetElevation = 30.0f;
    CurrentDistance = TargetDistance = 2000000.0f;

    // Constraints
    MinDistance = 800000.0f;
    MaxDistance = 15000000.0f;
    MinElevation = -80.0f;
    MaxElevation = 80.0f;

    // Movement
    RotationSpeed = 0.2f;
    ZoomSpeed = 0.1f; // Now proportional (10% per scroll)
    Damping = 8.0f;

    // State
    bAutoTargetPlanet = true;
    TargetLocation = FVector::ZeroVector;
    bIsOrbiting = false;
}

void APTPOrbitCamera::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoTargetPlanet)
    {
        FindPlanetTarget();
    }

    UpdateCameraPosition();
}

void APTPOrbitCamera::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsOrbiting)
    {
        OrbitCamera(DeltaTime);
    }

    // Handle azimuth wrapping for smooth interpolation
    float AzimuthDelta = TargetAzimuth - CurrentAzimuth;
    if (AzimuthDelta > 180.0f) CurrentAzimuth += 360.0f;
    else if (AzimuthDelta < -180.0f) CurrentAzimuth -= 360.0f;

    // Interpolate current toward target
    CurrentAzimuth = FMath::FInterpTo(CurrentAzimuth, TargetAzimuth, DeltaTime, Damping);
    CurrentElevation = FMath::FInterpTo(CurrentElevation, TargetElevation, DeltaTime, Damping);
    CurrentDistance = FMath::FInterpTo(CurrentDistance, TargetDistance, DeltaTime, Damping);

    // Wrap current azimuth to 0-360
    while (CurrentAzimuth > 360.0f) CurrentAzimuth -= 360.0f;
    while (CurrentAzimuth < 0.0f) CurrentAzimuth += 360.0f;

    UpdateCameraPosition();
}

void APTPOrbitCamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAction("OrbitCamera", IE_Pressed, this, &APTPOrbitCamera::StartOrbit);
    PlayerInputComponent->BindAction("OrbitCamera", IE_Released, this, &APTPOrbitCamera::StopOrbit);
    PlayerInputComponent->BindAxis("ZoomCamera", this, &APTPOrbitCamera::ZoomCamera);
    PlayerInputComponent->BindAction("ToggleCursor", IE_Pressed, this, &APTPOrbitCamera::ToggleCursorMode);
}

void APTPOrbitCamera::StartOrbit()
{
    bIsOrbiting = true;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC)
    {
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
        PC->bShowMouseCursor = true;
        PC->SetInputMode(FInputModeGameAndUI());
    }
}

void APTPOrbitCamera::OrbitCamera(float DeltaTime)
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    float MouseX, MouseY;
    PC->GetMousePosition(MouseX, MouseY);
    FVector2D CurrentMousePosition(MouseX, MouseY);

    FVector2D MouseDelta = CurrentMousePosition - LastMousePosition;

    TargetAzimuth += MouseDelta.X * RotationSpeed;
    TargetElevation -= MouseDelta.Y * RotationSpeed;

    TargetElevation = FMath::Clamp(TargetElevation, MinElevation, MaxElevation);

    while (TargetAzimuth > 360.0f) TargetAzimuth -= 360.0f;
    while (TargetAzimuth < 0.0f) TargetAzimuth += 360.0f;

    int32 ViewportSizeX, ViewportSizeY;
    PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
    LastMousePosition = FVector2D(ViewportSizeX / 2.0f, ViewportSizeY / 2.0f);
    PC->SetMouseLocation(LastMousePosition.X, LastMousePosition.Y);
}

void APTPOrbitCamera::ZoomCamera(float AxisValue)
{
    if (FMath::Abs(AxisValue) > 0.01f)
    {
        // Proportional zoom: 10% of current distance per scroll
        const float ProportionalSpeed = CurrentDistance * ZoomSpeed;
        TargetDistance -= AxisValue * ProportionalSpeed;
        TargetDistance = FMath::Clamp(TargetDistance, MinDistance, MaxDistance);
    }
}

void APTPOrbitCamera::UpdateCameraPosition()
{
    const float AzimuthRad = FMath::DegreesToRadians(CurrentAzimuth);
    const float ElevationRad = FMath::DegreesToRadians(CurrentElevation);

    const float HorizontalDistance = CurrentDistance * FMath::Cos(ElevationRad);
    const float X = HorizontalDistance * FMath::Cos(AzimuthRad);
    const float Y = HorizontalDistance * FMath::Sin(AzimuthRad);
    const float Z = CurrentDistance * FMath::Sin(ElevationRad);

    const FVector CameraOffset(X, Y, Z);
    const FVector NewCameraLocation = TargetLocation + CameraOffset;

    SetActorLocation(NewCameraLocation);

    const FVector DirectionToTarget = (TargetLocation - NewCameraLocation).GetSafeNormal();
    const FRotator NewCameraRotation = DirectionToTarget.Rotation();
    SetActorRotation(NewCameraRotation);
}

void APTPOrbitCamera::FindPlanetTarget()
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APTPPlanetActor::StaticClass(), FoundActors);

    if (FoundActors.Num() > 0)
    {
        TargetLocation = FoundActors[0]->GetActorLocation();
        UE_LOG(LogTemp, Log, TEXT("OrbitCamera: Auto-targeted planet at %s"), *TargetLocation.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("OrbitCamera: No planet found, using origin"));
        TargetLocation = FVector::ZeroVector;
    }
}

void APTPOrbitCamera::ToggleCursorMode()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

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
```

**File: `GaiaPTP/Public/PTPGameMode.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PTPGameMode.generated.h"

UCLASS()
class GAIAPTP_API APTPGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    APTPGameMode();
};
```

**File: `GaiaPTP/Private/PTPGameMode.cpp`**

```cpp
#include "PTPGameMode.h"
#include "PTPOrbitCamera.h"

APTPGameMode::APTPGameMode()
{
    DefaultPawnClass = APTPOrbitCamera::StaticClass();
}
```

**File: `Config/DefaultInput.ini` additions:**

```ini
[/Script/Engine.InputSettings]
+ActionMappings=(ActionName="OrbitCamera",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=LeftMouseButton)
+ActionMappings=(ActionName="OrbitCamera",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=RightMouseButton)
+ActionMappings=(ActionName="ToggleCursor",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=Escape)
+AxisMappings=(AxisName="ZoomCamera",Scale=1.000000,Key=MouseWheelAxis)
```

---

## Conclusion: The Feel of It All

And there we have it - a Spore-style orbital camera for viewing our tectonic planets!

The journey from "I want to look at a sphere" to "I have a polished, smooth, intuitive orbital camera" involved:
- Understanding spherical coordinates
- Converting between coordinate systems
- Handling input correctly (with mouse locking!)
- Adding smooth interpolation for that "weighted" feel
- Fixing edge cases like azimuth wrapping and proportional zoom

But the real lesson here is about **feel**. The difference between a camera that works and a camera that *feels good* comes down to subtle things: the damping factor, the zoom speed scaling, the constraint values. These are the parameters you'll want to tweak and iterate on until it feels just right for your project.

Try adjusting:
- `Damping` (8.0 is my sweet spot, but you might prefer 5.0 or 12.0)
- `RotationSpeed` (0.2 degrees per pixel feels good to me)
- `ZoomSpeed` (10% per scroll - maybe you want 15%?)
- `MinElevation` and `MaxElevation` (prevent camera from going underground or over-flipping)

The camera is now a *tool* - a window into your procedural world. And every time you orbit that planet, watching those 40 colored tectonic plates spin smoothly beneath your cursor, you'll feel a little bit of that Spore magic.

Now, let's get back to those tectonic simulations... 🌍

---

## Appendix: Build Checklist

**Files Created:**
- `Plugins/GaiaPTP/Source/GaiaPTP/Public/PTPOrbitCamera.h`
- `Plugins/GaiaPTP/Source/GaiaPTP/Private/PTPOrbitCamera.cpp`
- `Plugins/GaiaPTP/Source/GaiaPTP/Public/PTPGameMode.h`
- `Plugins/GaiaPTP/Source/GaiaPTP/Private/PTPGameMode.cpp`

**Files Modified:**
- `Config/DefaultInput.ini` (add input mappings)
- `Plugins/GaiaPTP/Source/GaiaPTP/GaiaPTP.Build.cs` (if needed - should auto-detect)

**Testing Steps:**
1. Build the project
2. Open editor, create new level (or use existing)
3. Place a `PTPPlanetActor` in the level
4. Set World Settings → GameMode → `PTPGameMode`
5. Hit Play
6. Click and drag to orbit
7. Scroll to zoom
8. Press Escape to get cursor back

**Expected Behavior:**
- Left/right mouse drag orbits smoothly around planet
- Mouse wheel zooms in/out with proportional speed
- Camera never goes below -80° or above +80° elevation
- Motion has pleasant damped inertia
- Escape toggles cursor mode

Enjoy your new planetary explorer! 🚀
