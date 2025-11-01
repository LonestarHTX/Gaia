# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Gaia** is an Unreal Engine 5.5 C++ project implementing a procedural tectonic planet generation system based on the research paper "Procedural Tectonic Planets" by Cortial et al. (2019). The project simulates geological processes (plate tectonics, erosion, mountain formation) to create geologically-plausible planets at multiple scales.

**Current Status:** ✅ **Phase 1 (Foundation) COMPLETE** - Full tectonic planet foundation implemented with Fibonacci sphere sampling, CGAL Delaunay triangulation, crust initialization, plate dynamics, boundary detection, parallelized initialization, Spore-style orbit camera, and comprehensive test suite. Ready for Phase 2 (plate movement and tectonic interactions).

## Quick Reference

**Most Common Commands:**
```powershell
# Quick build (no tests)
PowerShell ./Scripts/Build-PTP.ps1

# Build and run all tests (full validation)
PowerShell ./Scripts/BuildAndTest-PTP.ps1

# Setup CGAL (one-time, before first build)
PowerShell ./Scripts/Setup-CGAL.ps1
$env:VCPKG_ROOT = "$env:USERPROFILE\vcpkg"

# Launch editor
"C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\Michael\Documents\Unreal Projects\Gaia\Gaia.uproject"
```

**Key Files:**
- Plugin descriptor: `Plugins/GaiaPTP/GaiaPTP.uplugin`
- Settings: `Plugins/GaiaPTP/Source/GaiaPTP/Public/GaiaPTPSettings.h`
- Planet component: `Plugins/GaiaPTP/Source/GaiaPTP/Public/PTPPlanetComponent.h`
- Configuration: `Config/DefaultGame.ini`
- Implementation guide: `Documentation/Research/PTP/Implementation_Guide.md`

## Build Commands

### Initial Setup
```bash
# Generate Visual Studio project files (from project root)
"C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="C:\Users\Michael\Documents\Unreal Projects\Gaia\Gaia.uproject" -game -rocket -progress

# Or use UE5 command
"C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealVersionSelector-Win64-Shipping.exe" /projectfiles "C:\Users\Michael\Documents\Unreal Projects\Gaia\Gaia.uproject"
```

### Build from Command Line
```bash
# Build Development Editor (most common)
"C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" GaiaEditor Win64 Development "C:\Users\Michael\Documents\Unreal Projects\Gaia\Gaia.uproject" -waitmutex

# Build Shipping
"C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" Gaia Win64 Shipping "C:\Users\Michael\Documents\Unreal Projects\Gaia\Gaia.uproject" -waitmutex

# Clean build
"C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Clean.bat" GaiaEditor Win64 Development "C:\Users\Michael\Documents\Unreal Projects\Gaia\Gaia.uproject"
```

### Automated Build Scripts
```powershell
# Quick build (no tests) - use for parameter tweaks
PowerShell ./Scripts/Build-PTP.ps1

# Build and run all tests - use for feature changes
PowerShell ./Scripts/BuildAndTest-PTP.ps1

# BuildAndTest-PTP.ps1 runs:
# - GaiaPTP.Settings.Defaults
# - GaiaPTP.Component.DefaultsCopied
# - GaiaPTP.Fibonacci.CountAndRadius
# - GaiaPTP.Fibonacci.UniformityBins
# - GaiaPTP.Data.Defaults
# - GaiaPTP.Data.PlateVelocity
# - GaiaPTP.Seeding.Basic
# - GaiaPTP.Adjacency.Smoke
# - GaiaPTP.Adjacency.Integrity
# - GaiaPTP.Determinism.Sampling
# - GaiaPTP.Determinism.Seeding
# - GaiaPTP.CrustInit.DataInit
# - GaiaPTP.CrustInit.PlateDynamics
# - GaiaPTP.CrustInit.BoundaryDetection
# - GaiaPTP.CrustInit.Integration
```

### Build from Visual Studio
- Open `Gaia.sln`
- Set configuration to `Development Editor`
- Build → Build Solution (Ctrl+Shift+B)
- Run → Start Debugging (F5) to launch editor

### Running the Editor
```bash
# Launch editor directly
"C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\Michael\Documents\Unreal Projects\Gaia\Gaia.uproject"
```

### CGAL Setup (Required for Delaunay Triangulation)
```powershell
# Install CGAL and dependencies via vcpkg
PowerShell ./Scripts/Setup-CGAL.ps1

# Set environment variable (required before building)
$env:VCPKG_ROOT = "$env:USERPROFILE\vcpkg"

# The GaiaPTPCGAL module will detect CGAL headers and set WITH_CGAL=1
# If CGAL is not found, the module compiles but triangulation features are disabled
```

## Console Commands

Runtime console commands for development and debugging (accessible via ` key in-game):

### Profiling Commands
```
ptp.profile.start          - Begin CSV profiling capture for PTP category
ptp.profile.stop           - End CSV profiling capture
ptp.bench.rebuild          - Rebuild planet with current project settings (benchmark Phase 1)
ptp.bench.rebuild_np       - Rebuild with custom point/plate count (see cvars below)
```

### Configuration CVars
```
ptp.parallel 0/1           - Disable/enable ParallelFor in initialization (default: 1)
ptp.bench.numPoints N      - Set point count for ptp.bench.rebuild_np (default: 100000)
ptp.bench.numPlates N      - Set plate count for ptp.bench.rebuild_np (default: 40)
```

### Camera Controls (PTPOrbitCamera - Spore-style)
**Mouse:**
- Left/Right Mouse Drag - Orbit camera around planet
- Mouse Wheel - Zoom in/out
- Escape - Toggle cursor visibility

**Keyboard:**
- W/S or Up/Down - Tilt camera up/down
- A/D or Left/Right - Rotate camera left/right
- Q/E - Zoom out/in

### Example Profiling Session
```
# In-game console
ptp.profile.start
ptp.bench.rebuild
ptp.profile.stop
# CSV file written to: Saved/Profiling/
```

## Architecture

### High-Level System Design

The project follows a layered architecture designed to simulate and visualize tectonic planets:

**Layer 1: Foundation**
- **Sphere Representation**: 500,000 points sampled via Fibonacci spiral on sphere surface
- **Triangulation**: Spherical Delaunay triangulation via CGAL (✅ implemented, ~4 second build time)
- **Smart Rebuild**: Staleness detection caches triangulation - builds once in editor, PIE sessions use cached data (instant startup)
- **Data Structures**:
  - `FCrustData` - per-point elevation, thickness, type, age, ridge/fold directions
  - `FTectonicPlate` - rotation axis, angular velocity, terrane system
  - `FTerrane` - connected continental crust regions

**Layer 2: Tectonic Simulation**
- **Time-Stepped Evolution**: 2 million year discrete steps
- **Plate Movement**: Geodetic rotation (Euler pole kinematics)
- **Tectonic Interactions**:
  - Subduction (oceanic→continental with uplift formulas)
  - Continental collision (terrane transfer, Himalayan orogeny)
  - Seafloor spreading (new crust generation at divergent boundaries)
  - Plate rifting (stochastic Poisson-based fragmentation)
- **Surface Processes**: Erosion, oceanic dampening, sediment accretion

**Layer 3: Amplification**
- **Oceanic Detail**: 3D Gabor noise oriented by ridge direction and age
- **Continental Detail**: Exemplar-based synthesis using USGS SRTM90 real Earth data
- **Resolution**: Coarse simulation at ~50km → amplified to ~100m

**Layer 4: Visualization**
- **RealtimeMeshComponent**: High-performance dynamic mesh rendering
- **LOD System**: Multi-scale rendering from planetary to local detail
- **Material Assignment**: Based on crust type, orogeny type, and age

### Key Integration Points

**RealtimeMeshComponent Plugin** (`Plugins/RealtimeMeshComponent/`)
- Use `TRealtimeMeshBuilderLocal<uint32, ...>` for 500k+ vertex meshes
- Stream-based architecture for efficient updates
- Section system for multi-material plates
- Thread-safe async mesh updates

**CGAL Library** (`Plugins/GaiaPTP/Source/GaiaPTPCGAL/`)
- Required for `Delaunay_triangulation_on_sphere_2`
- Installed via vcpkg (see Setup-CGAL.ps1)
- Build system detects CGAL via VCPKG_ROOT environment variable
- Conditional compilation: WITH_CGAL=1 when available, WITH_CGAL=0 otherwise
- License: CGAL is GPL/LGPL (plugin module isolated to Win64 only)

### Simulation Constants (Appendix A from Paper)

All geological parameters are defined in the implementation guide:
- Time step: δt = 2 My
- Planet radius: R = 6370 km (Earth-scale)
- Max plate speed: v₀ = 100 mm/year
- Subduction uplift: u₀ = 6×10⁻⁷ km/year
- Continental erosion: εc = 3×10⁻⁵ mm/year
- Oceanic dampening: εo = 4×10⁻² mm/year

See `Documentation/Research/PTP/Implementation_Guide.md` for complete formulas and constants.

## Code Organization

### Module Structure

**Main Game Module** (`Source/Gaia/`)
- Minimal boilerplate - most logic now in GaiaPTP plugin
- Dependencies: Core, CoreUObject, Engine, InputCore, EnhancedInput

**GaiaPTP Plugin** (`Plugins/GaiaPTP/`)
Three-module architecture:

1. **GaiaPTP (Runtime)** (`Source/GaiaPTP/`)
   - Core simulation logic and data structures
   - Public headers in `Public/`: FibonacciSphere, TectonicTypes, TectonicData, TectonicSeeding, GaiaPTPSettings, PTPPlanetActor/Component, PTPDebugActor
   - Dependencies: Core, CoreUObject, Engine, RealtimeMeshComponent, GeometryCore, GeometryFramework, DeveloperSettings
   - Contains: FCrustData, FTerrane, FTectonicPlate structures; planet component; seeding algorithms

2. **GaiaPTPEditor (Editor)** (`Source/GaiaPTPEditor/`)
   - Editor tooling, property customization, asset actions
   - Build separately from runtime for editor-only features

3. **GaiaPTPCGAL (Runtime, Win64 only)** (`Source/GaiaPTPCGAL/`)
   - CGAL integration for Delaunay triangulation
   - Conditional build based on VCPKG_ROOT environment variable
   - Isolated module to contain GPL/LGPL licensing concerns

### Class Hierarchy (Implemented/Planned)

**Actor/Component System:**
- `APTPPlanetActor` - Placeable actor with default overrides
  - **IMPORTANT**: Place in **persistent level** (not streaming levels) to avoid load/unload cycles during camera movement
  - Auto-builds adjacency on first placement (~4s one-time operation)
  - Subsequent editor sessions and PIE use cached data (instant)
- `UPTPPlanetComponent` - Main component containing planet state and simulation logic
  - Points (500k FVector positions)
  - CrustData (500k FCrustData)
  - Triangles (Delaunay mesh via CGAL)
  - Plates (40-100 FTectonicPlate)
  - Simulation step methods

**Data Structures (Implemented):**
- `FCrustData` - per-point properties:
  - Type (Oceanic/Continental)
  - Elevation, Thickness, Age
  - RidgeDirection (oceanic)
  - FoldDirection, OrogenyType (continental)

- `FTectonicPlate` - plate state:
  - PointIndices
  - RotationAxis, AngularVelocity
  - Terranes (continental regions)

- `FTerrane` - continental crust regions

**Settings:**
- `UGaiaPTPSettings` - DeveloperSettings subclass
  - Configurable via Project Settings → Gaia → PTP
  - Persisted in `Config/DefaultGame.ini`
  - Contains all Appendix A constants

**Debug/Visualization:**
- `APTPDebugActor` - Debug visualization helpers

## Development Workflow

### Implementation Roadmap

The project follows a 6-phase build plan (see `Documentation/Research/PTP/Implementation_Guide.md`):

1. **Core Foundation** - Fibonacci sampling, CGAL integration, data structures
2. **Basic Simulation** - Plate movement, boundary detection, subduction
3. **Complete Tectonics** - All interactions, surface processes
4. **Optimization** - Profiling, multi-threading, spatial acceleration
5. **Amplification** - Gabor noise, exemplar synthesis, GPU shaders
6. **Polish** - UI controls, visualization, LOD, save/load

### Working with the Implementation Guide

**Primary Reference:** `Documentation/Research/PTP/Implementation_Guide.md`
- 2,475 lines of detailed implementation guidance
- Written as a "coding adventure" in Sebastian Lague style
- Contains verified formulas, pseudocode, and explanations
- Organized by checkpoints for incremental development

**Research Papers:**
- `Documentation/Research/PTP/2019-Procedural-Tectonic-Planets.pdf` - Primary source
- `Documentation/Research/RTHAP/` - Amplification techniques

### RealtimeMeshComponent Usage

**Key Documentation:**
- `Plugins/RealtimeMeshComponent/README.md` - Overview
- `Plugins/RealtimeMeshComponent/Docs/RMCDocs.md` - Full API reference
- `Plugins/RealtimeMeshComponent/Docs/RealtimeMeshComponent_HowTo.md` - Tutorials

**Common Patterns:**
```cpp
// Build mesh with 500k+ vertices
TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder;
Builder.EnableTangents();
Builder.EnableColors();

// Add vertices
for (auto& Point : Planet->Points) {
    Builder.AddVertex(Point)
        .SetNormalAndTangent(Normal, Tangent)
        .SetColor(Color);
}

// Update mesh (async-safe)
RealtimeMesh->UpdateSectionGroup(GroupKey, Builder.GetStreamSet());
```

### Testing and Validation

**Automation Test Framework:**
- Tests located in `GaiaPTP` module (runtime)
- Naming convention: `GaiaPTP.[Category].[TestName]`
- Run via UnrealEditor-Cmd.exe with `-ExecCmds="Automation RunTests [filter]; Quit"`

**Existing Test Coverage (15 tests):**
- `GaiaPTP.Settings.Defaults` - Verify DeveloperSettings initialization
- `GaiaPTP.Component.DefaultsCopied` - Test actor-level settings inheritance
- `GaiaPTP.Fibonacci.CountAndRadius` - Validate point count and sphere radius
- `GaiaPTP.Fibonacci.UniformityBins` - Check spatial distribution uniformity
- `GaiaPTP.Data.Defaults` - Test FCrustData, FTerrane, FTectonicPlate defaults
- `GaiaPTP.Data.PlateVelocity` - Verify plate rotation calculations
- `GaiaPTP.Seeding.Basic` - Test initial Voronoi plate partitioning
- `GaiaPTP.Adjacency.Smoke` - Basic adjacency/triangulation smoke test
- `GaiaPTP.Adjacency.Integrity` - Verify neighbor data bidirectional consistency
- `GaiaPTP.Determinism.Sampling` - Verify reproducible Fibonacci generation
- `GaiaPTP.Determinism.Seeding` - Verify reproducible plate seeding
- `GaiaPTP.CrustInit.DataInit` - Test crust data initialization
- `GaiaPTP.CrustInit.PlateDynamics` - Test plate rotation setup
- `GaiaPTP.CrustInit.BoundaryDetection` - Validate boundary flagging
- `GaiaPTP.CrustInit.Integration` - End-to-end initialization test

**Development Practice:**
- Add automation tests for each new feature per implementation guide checkpoints
- Tests verify both correctness and geological plausibility
- Use BuildAndTest-PTP.ps1 for rapid iteration (runs all 15 tests)

### WSL2 Considerations

Project path uses Windows path: `/mnt/c/Users/Michael/Documents/Unreal Projects/Gaia`
- Build commands must use Windows-style paths
- UE5 binaries are Windows executables
- PowerShell scripts run via Windows PowerShell, not pwsh
- CGAL vcpkg installation targets x64-windows triplet

## Critical Notes

### ✅ Completed (Phase 1 Foundation - COMPLETE)

**Core Systems:**
- ✅ GaiaPTP plugin with three-module architecture (Runtime, Editor, CGAL)
- ✅ UGaiaPTPSettings with Appendix A constants in Config/DefaultGame.ini
- ✅ Fibonacci sphere sampling (FFibonacciSphere) with uniformity validation
- ✅ CGAL Delaunay triangulation on sphere (spherical adjacency + triangles)
- ✅ Adjacency queries via CGAL provider interface

**Data Structures:**
- ✅ FCrustData - elevation, thickness, type, age, ridge/fold directions
- ✅ FTectonicPlate - rotation axis, angular velocity, terrane system
- ✅ FTerrane - continental crust regions
- ✅ APTPPlanetActor and UPTPPlanetComponent with per-actor settings override

**Initialization:**
- ✅ Plate seeding (Fibonacci-based Voronoi partitioning)
- ✅ Crust data initialization (continental/oceanic classification)
- ✅ Plate dynamics initialization (random rotation axes/velocities)
- ✅ Boundary detection (plate edge identification)
- ✅ Parallelization (ParallelFor in crust init + boundary detection)
- ✅ Performance logging (timing output for parallel operations)

**Visualization:**
- ✅ RealtimeMeshComponent integration (Points + Surface preview modes)
- ✅ Vertex-colored plate visualization (MurmurHash3 plate colors)
- ✅ Spore-style orbital camera (APTPOrbitCamera with mouse + keyboard controls)
- ✅ PTPGameMode (sets camera as default pawn)
- ✅ Material system (M_DevPlanet vertex color material)
- ✅ Smart rebuild workflow (4s first build, instant PIE sessions)

**Testing & Tooling:**
- ✅ Comprehensive automation test suite (15 tests passing):
  - Settings, Component, Fibonacci, Data, Seeding, Adjacency
  - Determinism tests (sampling + seeding reproducibility)
  - Crust init tests (data init, dynamics, boundaries, integration)
- ✅ Build scripts (Build-PTP.ps1, BuildAndTest-PTP.ps1)
- ✅ CGAL setup script (Setup-CGAL.ps1)
- ✅ Console commands (profiling, benchmarking, CVars)

### Phase 2 - Not Yet Implemented
- ❌ Plate movement kernel (geodetic rotation per time step)
- ❌ Tectonic interactions (subduction, collision, rifting, spreading)
- ❌ Surface processes (erosion, oceanic dampening)
- ❌ Amplification (Gabor noise, exemplar synthesis)
- ❌ LOD system for multi-scale rendering
- ❌ Save/load system for planet state

### Known Dependencies
- CGAL library - vcpkg-based installation via Setup-CGAL.ps1 (partial)
- USGS SRTM90 exemplar dataset - needed for Phase 6 continental amplification
- Modules already added: GeometryCore, GeometryFramework, RealtimeMeshComponent

### Performance Targets
- 500,000 sample points per planet
- ~37ms per 2My simulation time step (from paper benchmarks)
- Real-time mesh updates via RealtimeMeshComponent
- GPU-accelerated amplification shaders
- Multi-threaded plate operations

### Research Fidelity
This project implements the paper **faithfully** - use exact formulas, constants, and methods from the research. The implementation guide has verified all math and provides checkpoint validations. Do not simplify or approximate unless explicitly noted in the guide.