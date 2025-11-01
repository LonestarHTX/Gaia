# PTP Implementation Notes - Actual vs. Guide

This document tracks differences between the Implementation Guide and the actual code implementation for the Gaia project.

## Overview

The Implementation Guide (`Implementation_Guide.md`) provides a conceptual walkthrough in Sebastian Lague style. The actual implementation makes several improvements for production use. This document serves as a companion to help developers understand what was changed and why.

---

## Phase 1: Foundation (Tasks 1.1-1.8) ✅ COMPLETE

### 1.1 Fibonacci Sphere Sampling

**Guide Code (Line 96):**
```cpp
float Y = 1.0f - (2.0f * i) / (NumPoints - 1.0f);
```

**Actual Code (`FibonacciSphere.cpp`):**
```cpp
const double t = (static_cast<double>(i) + 0.5) / static_cast<double>(N);
const double y = 1.0 - 2.0 * t;
```

**Why Changed:**
- Avoids division by zero edge case when `NumPoints == 1`
- Better numerical properties (symmetric around 0.5)
- Uses double precision for intermediate calculations
- Mathematically equivalent for large N

**Status:** ✅ Improved implementation

---

### 1.2 Spherical Delaunay Triangulation

**Guide Code (Lines 137-169):**
```cpp
struct FDelaunayTriangle
{
    int32 Indices[3];      // Vertex indices
    int32 Neighbors[3];    // Neighboring triangle indices (-1 if boundary)
};

class FSphericalDelaunay
{
    static void Triangulate(...);  // Static method, returns void
};
```

**Actual Code (`IPTPAdjacencyProvider.h`, `CGALAdjacencyProvider.cpp`):**
```cpp
struct FPTPAdjacency
{
    TArray<TArray<int32>> Neighbors;  // Vertex-to-vertex neighbors
    TArray<FIntVector> Triangles;     // Triangle vertex indices only
};

class IPTPAdjacencyProvider
{
    virtual bool Build(..., FString& OutError) = 0;  // Interface, returns success
};
```

**Why Changed:**
- **Interface pattern** for multiple implementations (CGAL, fallback, future alternatives)
- **Error handling** with bool return and error string
- **Simplified triangle storage** - dropped `Neighbors[3]` (triangle-to-triangle adjacency)
  - Only needed vertex-to-vertex neighbors for boundary detection
  - Reduces memory footprint
- **Graceful degradation** when CGAL unavailable
- **DLL loading** for vcpkg dependencies (gmp, mpfr)

**Status:** ✅ Production-ready with better architecture

---

### 1.3 Data Structures

**Guide Code (Lines 186-279):**
```cpp
struct FCrustData
{
    ECrustType Type;
    float Thickness;
    // ... plain C++ struct
};
```

**Actual Code (`TectonicTypes.h`, `TectonicData.h`):**
```cpp
UENUM(BlueprintType)
enum class ECrustType : uint8 { Oceanic, Continental };

USTRUCT(BlueprintType)
struct FCrustData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Crust")
    ECrustType Type;
    // ... all fields with UPROPERTY markup
};
```

**Why Changed:**
- **Full Unreal integration** - USTRUCT, UPROPERTY, UENUM markup
- **Blueprint compatibility** - expose to visual scripting
- **Editor support** - properties visible in Details panel
- **Reflection system** - serialization, networking, replication support
- **Organization** - split into TectonicTypes.h (enums) and TectonicData.h (structs)

**Additional Fields:**
- `FTectonicPlate::CentroidDir` - Store original seed position for debugging

**Status:** ✅ Enhanced with full Unreal integration

---

### 1.4 Settings System

**Guide Code:**
- No dedicated settings system mentioned
- Constants hardcoded in planet class

**Actual Code (`GaiaPTPSettings.h`):**
```cpp
UCLASS(Config=Game, DefaultConfig, MinimalAPI, meta=(DisplayName="PTP Settings"))
class UGaiaPTPSettings : public UDeveloperSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Config, Category="Planet")
    float PlanetRadiusKm = 6370.0f;

    UPROPERTY(EditAnywhere, Config, Category="Simulation")
    float TimeStepMy = 2.0f;

    // ... all Appendix A constants
};
```

**Why Added:**
- **Project-level configuration** via UDeveloperSettings
- **Editor UI** at Project Settings → Gaia → PTP
- **Persistence** in Config/DefaultGame.ini
- **Per-actor overrides** in UPTPPlanetComponent
- **Two-tier system**: Global defaults + actor-specific customization
- **Version control friendly** - settings in text config files

**Additional Settings:**
- `InitialSeed` - For deterministic randomness
- `DebugDrawStride` - Performance tuning for visualization
- `bAllowDynamicResample` - Runtime point regeneration

**Status:** ✅ Major production enhancement (not in guide)

---

### 1.5 Initial Plate Generation

**Guide Code (Lines 351-457):**
```cpp
// Random centroids
for (int32 i = 0; i < NumPlates; i++)
{
    float Theta = FMath::FRandRange(0.0f, 2.0f * PI);
    float Phi = FMath::Acos(FMath::FRandRange(-1.0f, 1.0f));
    FVector Centroid = /* spherical to cartesian */;
}

// Voronoi assignment with Euclidean distance
float Dist = FVector::Dist(Points[PointIdx], PlateCentroids[PlateIdx]);
```

**Actual Code (`TectonicSeeding.cpp`):**
```cpp
// Fibonacci-distributed seeds (deterministic)
void FTectonicSeeding::GeneratePlateSeeds(int32 NumPlates, TArray<FVector>& OutSeeds)
{
    TArray<FVector> Temp;
    FFibonacciSphere::GeneratePoints(NumPlates, 1.0f, Temp);
    for (const FVector& P : Temp)
    {
        OutSeeds.Add(P.GetSafeNormal());
    }
}

// Voronoi assignment with geodesic distance
void FTectonicSeeding::AssignPointsToSeeds(...)
{
    const FVector Pn = Points[i].GetSafeNormal();
    float BestDot = -FLT_MAX;
    for (int32 j = 0; j < M; ++j)
    {
        const float D = FVector::DotProduct(Pn, Seeds[j]);
        if (D > BestDot) { BestDot = D; BestIdx = j; }
    }
}
```

**Why Changed:**
1. **Fibonacci seeds instead of random**
   - More uniform plate sizes
   - Deterministic and reproducible
   - Better for automated testing
   - Eliminates rare cases of clustered plates

2. **Geodesic distance instead of Euclidean**
   - Mathematically correct for spherical geometry
   - Uses dot product (maximize = minimize great-circle distance)
   - Avoids distortion near poles
   - Computationally faster (no sqrt needed)

**Status:** ✅ Algorithmically superior to guide

---

### 1.6 Planet Component Architecture

**Guide Code:**
```cpp
class UTectonicPlanet : public UObject
{
    UPROPERTY(EditAnywhere, Category = "Planet")
    float PlanetRadius = 6370.0f;

    TArray<FVector> Points;
    TArray<FCrustData> CrustData;
    TArray<FDelaunayTriangle> Triangles;
    TArray<TArray<int32>> PointNeighbors;
    TArray<FTectonicPlate> Plates;
    float CurrentTime = 0.0f;

    void Initialize();
};
```

**Actual Code (`PTPPlanetComponent.h`):**
```cpp
UCLASS(ClassGroup=(Gaia), meta=(BlueprintSpawnableComponent))
class UPTPPlanetComponent : public UActorComponent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category="PTP|Config")
    bool bUseProjectDefaults = true;  // NEW: Toggle inheritance

    // All settings duplicated as per-actor overrides
    UPROPERTY(EditAnywhere, Category="PTP|Planet")
    float PlanetRadiusKm;
    // ... [all UGaiaPTPSettings fields duplicated]

    // Runtime data (Transient = not serialized)
    TArray<FVector> SamplePoints;

    UPROPERTY(VisibleAnywhere, Transient, Category="PTP|Debug")
    TArray<int32> PointPlateIds;  // Flat array, not nested in plates

    UPROPERTY(VisibleAnywhere, Transient, Category="PTP|Debug")
    TArray<FTectonicPlate> Plates;

    TArray<TArray<int32>> Neighbors;

    UPROPERTY(VisibleAnywhere, Transient, Category="PTP|Debug")
    TArray<FIntVector> Triangles;

    UFUNCTION(BlueprintCallable, Category="PTP")
    void ApplyDefaultsFromProjectSettings();

    UFUNCTION(BlueprintCallable, Category="PTP")
    void RebuildPlanet();
};
```

**Why Changed:**
- **UActorComponent instead of UObject** - Actor-based usage, placeable in levels
- **Two-tier settings** - Global + per-actor overrides with `bUseProjectDefaults` toggle
- **Transient data** - Runtime arrays not serialized (yet - waiting for Phase 2)
- **Flat plate assignment** - `PointPlateIds[i]` instead of nested in plate structs
  - Better cache locality for iteration
  - Easier to update during simulation
- **Blueprint functions** - `RebuildPlanet()`, `ApplyDefaultsFromProjectSettings()`
- **CSV profiling** - Performance monitoring in `RebuildPlanet()`

**Companion Actor:**
```cpp
UCLASS()
class APTPPlanetActor : public AActor
{
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UPTPPlanetComponent* PlanetComponent;
};
```

**Status:** ✅ Production architecture (guide uses simpler UObject)

---

### 1.7 Visualization

**Guide Code (Lines 477-521):**
```cpp
void UpdatePlanetMesh(UTectonicPlanet* Planet)
{
    TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder;
    Builder.EnableTangents();
    Builder.EnableColors();

    for (int32 i = 0; i < Planet->Points.Num(); i++)
    {
        FLinearColor Color = (Crust.Type == ECrustType::Continental)
            ? FLinearColor(0.6f, 0.5f, 0.3f)  // Tan
            : FLinearColor(0.1f, 0.2f, 0.5f); // Blue

        Builder.AddVertex(Position).SetNormalAndTangent(...).SetColor(Color);
    }

    RealtimeMesh->UpdateSectionGroup(SectionGroupKey, Builder.GetStreamSet());
}
```

**Actual Code (`PTPPlanetActor.cpp`):**
```cpp
void APTPPlanetActor::RebuildMesh()
{
    TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder;
    Builder.EnableTangents();
    Builder.EnableColors();

    const float Scale = Planet->VisualizationScale;

    // Build vertex buffer
    for (int32 i = 0; i < Pts.Num(); ++i)
    {
        const FVector N = Pts[i].GetSafeNormal();
        const FVector P = Pts[i] * Scale;  // Apply visualization scale
        const FColor C = PlateIds.IsValidIndex(i) ? PlateColor(PlateIds[i]) : FColor::Cyan;

        Builder.AddVertex(FVector3f(P))
            .SetNormalAndTangent(FVector3f(N), FVector3f(T))
            .SetColor(C);
    }

    // Build index buffer (double-sided triangles)
    for (const FIntVector& Tri : Planet->Triangles)
    {
        Builder.AddTriangle(ia, ib, ic);  // Front
        Builder.AddTriangle(ic, ib, ia);  // Back
    }

    RMSimple->CreateSectionGroup(SurfaceGroupKey, StreamSet);
    RealtimeMesh->SetMaterial(0, PlanetMaterial);
}
```

**Why Changed:**
- **Per-plate coloring** instead of crust type (crust data not yet used for visualization)
- **MurmurHash3** for deterministic plate colors
- **Double-sided triangles** for preview reliability
- **VisualizationScale** parameter (default 100x for UE units: 6370km → 637,000 units)
- **Two preview modes**: Points (debug) and Surface (final)
- **M_DevPlanet material** - Vertex color material with basic lighting

**Additional Features:**
- **Smart rebuild** - Only regenerate when parameters change
- **OnConstruction auto-build** - Mesh appears when actor added to level
- **Culling disabled** - Planet always visible (SetCullDistance(0))

**Status:** ✅ Implemented with plate visualization (crust-based colors deferred to Phase 2)

---

### 1.8 Testing and Validation

**Guide Code:**
- Has "Self-Check" narrative sections
- Suggests manual validation
- No automation test code

**Actual Code:**
**15 Automation Tests Implemented:**

**Settings & Component (2 tests):**
1. **GaiaPTP.Settings.Defaults** - Verify UGaiaPTPSettings initialization
2. **GaiaPTP.Component.DefaultsCopied** - Test actor-level settings inheritance

**Fibonacci Sampling (2 tests):**
3. **GaiaPTP.Fibonacci.CountAndRadius** - Validate point count and radius
4. **GaiaPTP.Fibonacci.UniformityBins** - Check spatial distribution uniformity

**Data Structures (2 tests):**
5. **GaiaPTP.Data.Defaults** - Test FCrustData, FTerrane, FTectonicPlate defaults
6. **GaiaPTP.Data.PlateVelocity** - Verify v = ω × p formula

**Plate Seeding (1 test):**
7. **GaiaPTP.Seeding.Basic** - Test Voronoi plate partitioning

**Adjacency (2 tests):**
8. **GaiaPTP.Adjacency.Smoke** - Basic triangulation smoke test
9. **GaiaPTP.Adjacency.Integrity** - Verify neighbor data consistency

**Determinism (2 tests):**
10. **GaiaPTP.Determinism.Sampling** - Verify reproducible Fibonacci generation
11. **GaiaPTP.Determinism.Seeding** - Verify reproducible plate seeding

**Crust Initialization (4 tests):**
12. **GaiaPTP.CrustInit.DataInit** - Test crust data initialization
13. **GaiaPTP.CrustInit.PlateDynamics** - Test plate rotation setup
14. **GaiaPTP.CrustInit.BoundaryDetection** - Validate boundary flagging
15. **GaiaPTP.CrustInit.Integration** - End-to-end initialization test

**Why Added:**
- **Regression prevention** - Catch bugs during development
- **Mathematical validation** - Verify properties (uniformity, geometry)
- **CI/CD ready** - Automated via BuildAndTest-PTP.ps1
- **Documentation** - Tests show expected behavior
- **Confidence** - Numerical validation beyond visual inspection

**Status:** ✅ Comprehensive test coverage (15 tests passing)

---

## Build System

### CGAL Integration

**Guide Code:**
```cpp
// TODO: CGAL integration implementation
```

**Actual Implementation:**

**Setup Script (`Scripts/Setup-CGAL.ps1`):**
```powershell
# Install via vcpkg
vcpkg install cgal:x64-windows gmp:x64-windows mpfr:x64-windows

# Set environment variable
$env:VCPKG_ROOT = "$env:USERPROFILE\vcpkg"
```

**Build System (`GaiaPTPCGAL.Build.cs`):**
```csharp
string VcpkgRoot = Environment.GetEnvironmentVariable("VCPKG_ROOT");
if (!string.IsNullOrEmpty(VcpkgRoot))
{
    PublicDefinitions.Add("WITH_PTP_CGAL_LIB=1");
    PublicIncludePaths.Add(Path.Combine(VcpkgRoot, "installed/x64-windows/include"));
    // Add lib paths and DLL dependencies...
}
else
{
    PublicDefinitions.Add("WITH_PTP_CGAL_LIB=0");
}
```

**Runtime (`CGALAdjacencyProvider.cpp`):**
```cpp
#if WITH_PTP_CGAL_LIB
    // Load GMP/MPFR DLLs manually
    HMODULE GmpDll = LoadLibraryW(L"libgmp-10.dll");
    HMODULE MpfrDll = LoadLibraryW(L"libmpfr-6.dll");

    // Call external C function
    int Result = ptp_cgal_triangulate(InData.data(), N, OutTriangles, OutEdges);
#else
    OutError = TEXT("CGAL support not compiled (VCPKG_ROOT not set)");
    return false;
#endif
```

**Why This Approach:**
- **Conditional compilation** - WITH_PTP_CGAL_LIB for graceful degradation
- **Environment variable** - VCPKG_ROOT detection at build time
- **Manual DLL loading** - Avoids static linking GPL/LGPL code
- **Isolated module** - GaiaPTPCGAL contains all licensing concerns
- **Fallback support** - Tests report skip if CGAL unavailable

**Status:** ✅ Production-ready CGAL integration

---

### Build Automation

**Guide Code:**
- No build automation mentioned

**Actual Scripts (3 automation scripts):**

**1. BuildAndTest-PTP.ps1** - Full validation
```powershell
# Build GaiaEditor Win64 Development
& "C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" `
    GaiaEditor Win64 Development "..." -waitmutex

# Run all 15 GaiaPTP tests
& "C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
    "..." -ExecCmds="Automation RunTests GaiaPTP; Quit" -NullRHI -Unattended
```

**2. Build-PTP.ps1** - Quick build (no tests)
```powershell
# Build only, skip tests for faster iteration
& "C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" `
    GaiaEditor Win64 Development "..." -waitmutex
```

**3. Bench-PTP.ps1** - Benchmarking
```powershell
# Profile rebuild performance with CSV capture
# Uses console commands: ptp.profile.start, ptp.bench.rebuild, ptp.profile.stop
```

**Why Added:**
- **One-command workflow** - Build + test in single script
- **CI/CD ready** - Exit codes for success/failure
- **Developer productivity** - Rapid iteration without opening IDE (Build-PTP.ps1)
- **Continuous validation** - Run after every change
- **Performance monitoring** - Benchmarking script for profiling

**Status:** ✅ Production enhancement (3 scripts)

---

### 1.9 Crust Initialization System (New)

**Guide Code:**
- No crust initialization implementation
- Deferred to Phase 2

**Actual Code (`CrustInitialization.h/cpp`):**
```cpp
class FCrustInitialization
{
public:
    // Initialize crust data for all points (parallelized per-plate)
    static void InitializeCrustData(
        const TArray<FVector>& SamplePoints,
        const TArray<TArray<int32>>& PlateToPoints,
        float ContinentalRatio,
        float AbyssalPlainElevationKm,
        float HighestOceanicRidgeElevationKm,
        int32 Seed,
        TArray<FCrustData>& OutCrustData);

    // Initialize plate dynamics (rotation axes and velocities)
    static void InitializePlateDynamics(
        int32 NumPlates,
        float PlanetRadiusKm,
        float MaxPlateSpeedMmPerYear,
        int32 Seed,
        TArray<FTectonicPlate>& OutPlates);

    // Detect plate boundaries (points with neighbors on different plates)
    static void DetectPlateBoundaries(
        const TArray<int32>& PointPlateIds,
        const TArray<TArray<int32>>& Neighbors,
        TArray<bool>& OutIsBoundaryPoint);

    // Classify plates as continental or oceanic
    static void ClassifyPlates(
        int32 NumPlates,
        float ContinentalRatio,
        int32 Seed,
        TArray<bool>& OutIsPlateContinent);
};
```

**Implementation Details:**
- **Continental crust**: 35km thick, ~0.5km elevation, random orogeny age (500-3000 My)
- **Oceanic crust**: 7km thick, elevation by distance from plate center (-1 to -6km), age 0-200 My
- **Parallelization**: Per-plate ParallelFor with deterministic per-plate RNG
- **Performance logging**: ~2ms for 40 plates (parallelized)
- **Fisher-Yates shuffle**: For random continental/oceanic classification

**Why Added:**
- **Phase 1 completion** - All data structures now fully initialized
- **Parallelization** - Performance optimization for large plate counts
- **Determinism**: Per-plate seeds ensure reproducible results
- **Geological plausibility** - Follows paper's Appendix A constants

**Status:** ✅ Complete crust initialization (early Phase 1 completion)

---

### 1.10 Orbit Camera System (New)

**Guide Code:**
- No camera system mentioned

**Actual Code (`PTPOrbitCamera.h/cpp`, `PTPGameMode.h/cpp`):**
```cpp
UCLASS()
class APTPOrbitCamera : public APawn
{
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    UCameraComponent* Camera;

    // Spherical coordinates
    float CurrentAzimuth = 45.0f;
    float CurrentElevation = 30.0f;
    float CurrentDistance = 2000000.0f;  // ~3x planet radius

    // Constraints
    float MinDistance = 800000.0f;   // ~1.25x radius
    float MaxDistance = 15000000.0f; // ~23x radius
    float MinElevation = -80.0f;
    float MaxElevation = 80.0f;

    // Movement settings
    float RotationSpeed = 0.2f;  // Degrees per pixel
    float ZoomSpeed = 0.1f;      // Proportion per scroll
    float Damping = 8.0f;        // Interpolation speed

    void StartOrbit();   // Mouse button pressed
    void StopOrbit();    // Mouse button released
    void OrbitCamera(float DeltaTime);
    void ZoomCamera(float AxisValue);
    void UpdateCameraPosition();
};

UCLASS()
class APTPGameMode : public AGameModeBase
{
    APTPGameMode()
    {
        DefaultPawnClass = APTPOrbitCamera::StaticClass();
    }
};
```

**Features:**
- **Mouse drag** - Orbit around planet (infinite rotation with cursor lock)
- **Mouse wheel** - Proportional zoom (10% of current distance)
- **Keyboard** - WASD/arrows for rotation, Q/E for zoom
- **Smooth interpolation** - FMath::FInterpTo with damping
- **Auto-targeting** - Finds PTPPlanetActor automatically
- **Azimuth wrapping** - Handles 0-360 degree wraparound correctly

**Why Added:**
- **Visualization** - Essential for viewing planet during development
- **Spore inspiration** - Familiar controls from Spore's creature creator
- **No external dependencies** - Self-contained camera system
- **Editor integration** - Works in PIE and packaged builds

**Documentation:** `Documentation/SporeCamera_Implementation_Guide.md`

**Status:** ✅ Full orbit camera implementation

---

### 1.11 Smart Rebuild System (New)

**Guide Code:**
- No caching or staleness detection

**Actual Code (`PTPPlanetActor.cpp`):**
```cpp
void APTPPlanetActor::BeginPlay()
{
    // Smart rebuild: only regenerate if data is missing or stale
    const bool bNeedsRebuild = (Planet->SamplePoints.Num() != Planet->NumSamplePoints);
    const bool bNeedsAdjacency = (Planet->Triangles.Num() == 0);

    if (bNeedsRebuild)
    {
        UE_LOG(LogTemp, Warning, TEXT("PTP: Rebuilding planet (sample count changed)"));
        Planet->RebuildPlanet();
        BuildAdjacency();
    }
    else if (bNeedsAdjacency)
    {
        UE_LOG(LogTemp, Warning, TEXT("PTP: Building missing adjacency"));
        BuildAdjacency();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("PTP: Using cached planet data - no rebuild needed"));
    }

    // Always rebuild mesh (lightweight)
    RebuildMesh();
}

void APTPPlanetActor::OnConstruction(const FTransform& Transform)
{
    // Mirror BeginPlay logic for editor workflow
    // Adjacency builds automatically when actor added to level
}
```

**Why Added:**
- **Problem**: 4-second triangulation ran on every PIE session
- **Solution**: Check data staleness before rebuilding
- **Result**: One-time build in editor, instant PIE startup
- **Performance**: First run ~4s, subsequent runs <100ms

**Status:** ✅ Eliminates redundant expensive operations

---

### 1.12 PIE Data Persistence (New)

**Guide Code:**
- No discussion of editor/PIE workflow

**Actual Code (`PTPPlanetComponent.h`):**
```cpp
// Runtime planet data - not exposed in Details panel but duplicated to PIE for instant startup
// Note: Removing Transient allows PIE duplication while keeping them hidden (no EditAnywhere/VisibleAnywhere)
UPROPERTY()
TArray<FVector> SamplePoints;

UPROPERTY()
TArray<int32> PointPlateIds;

UPROPERTY()
TArray<FCrustData> CrustData;

UPROPERTY()
TArray<FIntVector> Triangles;
```

**Why Changed:**
- **Problem**: `Transient` property prevented PIE duplication
- **Solution**: Use bare `UPROPERTY()` for GC + duplication without UI exposure
- **Result**: Editor-cached triangulation now transfers to PIE instantly
- **Professional workflow**: Build once in editor, iterate quickly in PIE

**Status:** ✅ Instant PIE startup with cached data

---

### 1.13 Performance Logging (New)

**Guide Code:**
- No performance monitoring

**Actual Code (`CrustInitialization.cpp`):**
```cpp
const double StartTime = FPlatformTime::Seconds();
if (bDoParallel)
{
    ParallelFor(NumPlates, WorkPerPlate);
    const double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
    UE_LOG(LogGaiaPTP, Log, TEXT("Crust init: %d plates parallelized in %.2fms"), NumPlates, ElapsedMs);
}
else
{
    for (int32 PlateIdx = 0; PlateIdx < NumPlates; ++PlateIdx) { WorkPerPlate(PlateIdx); }
    const double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
    UE_LOG(LogGaiaPTP, Log, TEXT("Crust init: %d plates sequential in %.2fms"), NumPlates, ElapsedMs);
}
```

**Features:**
- **FPlatformTime::Seconds()** - High-resolution timing
- **Parallel vs sequential** - Compare parallelization effectiveness
- **Console output** - Visible in editor Output Log
- **CSV profiling** - Console commands for profiling capture

**Console Commands:**
- `ptp.profile.start` - Begin CSV profiling capture
- `ptp.profile.stop` - End CSV profiling capture
- `ptp.bench.rebuild` - Rebuild planet with timing
- `ptp.parallel 0/1` - Disable/enable ParallelFor

**Why Added:**
- **Optimization verification** - Prove parallelization is working
- **Performance regression** - Catch slowdowns during development
- **Tuning guidance** - Identify bottlenecks

**Status:** ✅ Comprehensive performance monitoring

---

## Module Architecture

**Guide Code:**
- Assumes monolithic implementation
- No discussion of module separation

**Actual Architecture:**

```
Plugins/GaiaPTP/
├── GaiaPTP.uplugin
└── Source/
    ├── GaiaPTP/           (Runtime)
    │   ├── Public/        Core simulation, data structures
    │   └── Private/       Tests, implementation
    ├── GaiaPTPEditor/     (Editor)
    │   ├── Public/        Property customizers
    │   └── Private/       Asset actions, tooling
    └── GaiaPTPCGAL/       (Runtime, Win64 only)
        ├── Public/        Interface definitions
        └── Private/       CGAL implementation, tests
```

**Why This Structure:**
1. **Licensing isolation** - CGAL (GPL/LGPL) in separate module
2. **Editor separation** - No editor code in runtime builds
3. **Platform isolation** - CGAL module Win64-only
4. **Clean dependencies** - GaiaPTP doesn't depend on GaiaPTPCGAL directly
5. **Interface pattern** - Access via IPTPAdjacencyProvider

**Status:** ✅ Production best practice

---

## Summary of Improvements

### Mathematical/Algorithmic
- ✅ Fibonacci seeding > Random seeding (more uniform)
- ✅ Geodesic distance > Euclidean distance (correct geometry)
- ✅ Improved Fibonacci formula (better numerical properties)

### Architecture
- ✅ UActorComponent > UObject (actor-based usage)
- ✅ Interface pattern for CGAL (better abstraction)
- ✅ Two-tier settings (project + per-actor)
- ✅ Three-module plugin (licensing, separation of concerns)

### Production Features
- ✅ Comprehensive automation tests (15 tests)
- ✅ Build/test automation scripts (3 scripts)
- ✅ CGAL integration with graceful degradation
- ✅ Full Blueprint support (UPROPERTY markup)
- ✅ CSV profiling for performance monitoring
- ✅ Error handling with detailed messages
- ✅ Mesh visualization (RealtimeMeshComponent with per-plate colors)
- ✅ Crust data initialization (FCrustInitialization class)
- ✅ Orbit camera system (Spore-style controls)
- ✅ Smart rebuild (staleness detection)
- ✅ PIE data persistence (instant startup)
- ✅ Performance logging (parallel operation timing)

### Deferred to Phase 2
- ❌ Crust-based visualization (currently using per-plate colors)
- ❌ Plate movement kernel (geodetic rotation)
- ❌ Tectonic interactions (subduction, collision, rifting, spreading)
- ❌ Surface processes (erosion, oceanic dampening)
- ❌ Simulation loop (time-stepping)

---

## Recommendations for Guide Updates

If updating the Implementation Guide to match reality:

1. **Lines 77-114 (Fibonacci)**: Update formula to `(i + 0.5)/N`
2. **Lines 137-169 (Triangulation)**: Change to interface pattern
3. **Lines 186-279 (Data Structures)**: Add UPROPERTY markup discussion
4. **Lines 351-457 (Seeding)**: Change to Fibonacci seeds + geodesic distance
5. **After Line 293**: Insert new "Settings System" section
6. **After Line 530**: Add "Automation Testing" section
7. **Lines 567-583**: Add module architecture discussion

However, the guide serves its purpose well as conceptual documentation. This companion document bridges the gap between concept and implementation.

---

## Phase 2: Ready to Begin

**Phase 1 Complete** - All foundation systems implemented:
- ✅ Fibonacci sphere sampling
- ✅ CGAL Delaunay triangulation
- ✅ Complete data structures
- ✅ Settings system (two-tier)
- ✅ Crust data initialization
- ✅ Plate dynamics initialization
- ✅ Boundary detection
- ✅ Mesh visualization (per-plate colors)
- ✅ Orbit camera
- ✅ Smart rebuild + PIE persistence
- ✅ 15 automation tests
- ✅ Build automation scripts

**Phase 2 Focus** - Plate movement and tectonic interactions:

1. **Plate Movement Kernel**
   - Geodetic rotation per time step
   - Update point positions based on plate velocities
   - Rodrigues' rotation formula: v = ω × r

2. **Boundary Classification**
   - Classify edges as convergent/divergent/transform
   - Use relative velocity dot product with boundary normal
   - Mark oceanic-continental vs oceanic-oceanic convergence

3. **Subduction (First Tectonic Interaction)**
   - Detect oceanic-continental convergence
   - Apply uplift formula from paper (Eq. 2-4)
   - Trench formation (lower oceanic crust)

4. **Continental Collision**
   - Terrane transfer across boundaries
   - Fold mountain formation (Himalayan orogeny)
   - Slab break-off detection

5. **Seafloor Spreading**
   - Generate new oceanic crust at divergent boundaries
   - Age gradient from ridge (0 My at center)
   - Elevation based on age (subsidence model)

6. **Simulation Loop**
   - Time-stepping (2 My per step)
   - Editor controls (pause/step/run)
   - State persistence (save/load)

See `Documentation/Research/PTP/Implementation_Guide.md` (Phase 2 section) for detailed formulas and implementation guidance.
