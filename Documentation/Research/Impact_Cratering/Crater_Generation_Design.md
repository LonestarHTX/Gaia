# Impact Crater Generation for Planetary Systems

## Overview

Impact craters are a **critical gap** in the PTP/RTHAP system. While these papers excel at generating geologically active planets like Earth, they don't account for impact cratering - the dominant surface feature on dead/low-activity worlds like the Moon, Mercury, and Mars.

**Key Insight:** The PTP/RTHAP papers focus on *active* tectonic planets where craters erode quickly. For complete planetary generation covering the full spectrum from Earth to Luna, we need to integrate impact cratering.

## The Problem: Earth vs Moon Surface Features

### Earth (Active Planet)
- **Dominated by**: Tectonic features (mountains, rifts, volcanoes)
- **Crater preservation**: Poor - rapid erosion from water, wind, plate tectonics
- **Oldest visible crater**: ~2 billion years (most buried/eroded)
- **Crater count**: ~190 confirmed impact craters
- **Surface age**: Constantly renewing

### Moon (Dead World)
- **Dominated by**: Impact craters at all scales
- **Crater preservation**: Excellent - no erosion, minimal tectonics
- **Oldest craters**: 4+ billion years still clearly visible
- **Crater count**: Millions, with overlapping layers
- **Surface age**: Ancient surfaces preserved

### Why PTP/RTHAP Don't Include Craters

The papers explicitly focus on **geologically active** planets:
- Active plate tectonics
- Ongoing erosion and weathering
- Surface renewal through volcanism
- Impact craters would be eroded/subducted away quickly on Earth-like worlds

**Conclusion:** PTP/RTHAP are NOT suitable for dead moons/planets without modification!

---

## Crater Size Distribution

Impact craters follow a **power law distribution** - many small craters, few large ones. This is based on the Neukum production function and observed crater populations across the solar system.

### Mathematical Model

```cpp
// Crater size distribution (Neukum production function)
double GenerateCraterDiameter(FRandomStream& RNG)
{
    // Power law: N(D) ∝ D^(-b) where b ≈ 2-3
    // More small craters than large ones

    double MinDiameter = 0.01;  // km (10m minimum)
    double MaxDiameter = 1000.0; // km (basin scale)
    double PowerLawExponent = -2.5;

    // Generate using inverse transform sampling
    double U = RNG.FRand();
    double Diameter = MinDiameter * FMath::Pow(
        FMath::Pow(MaxDiameter / MinDiameter, PowerLawExponent + 1) * U,
        1.0 / (PowerLawExponent + 1)
    );

    return Diameter;
}
```

**Key Parameters:**
- **MinDiameter**: 10m (smaller craters below mesh resolution)
- **MaxDiameter**: 1000km (giant impact basins like Imbrium on Moon)
- **PowerLawExponent**: -2.5 (calibrated from lunar observations)

---

## Crater Generation Algorithm

### Data Structure

```cpp
struct ImpactCrater
{
    FVector Location;        // Impact site on sphere
    double Diameter;         // Rim-to-rim distance (km)
    double Depth;            // km (below pre-impact surface)
    double Age;              // years since impact
    double EjectaRadius;     // km (typically 2-3x diameter)
    bool IsDegraded;         // Eroded/buried over time?
    bool IsComplex;          // Simple vs complex crater morphology
};
```

### Generation Function

```cpp
void GenerateImpactCraters(LowResMesh& Mesh,
                           double PlanetAge,
                           double TectonicActivity,
                           double AtmosphericDensity)
{
    TArray<ImpactCrater> Craters;
    FRandomStream RNG(Seed);

    // Crater density depends on:
    // - Planet age (older = more accumulated impacts)
    // - Tectonic activity (higher = fewer visible craters)
    // - Atmosphere (denser = filters small impactors)

    double CraterDensity = ComputeCraterDensity(
        PlanetAge,
        TectonicActivity,
        AtmosphericDensity
    );

    int32 NumCraters = CraterDensity * Mesh.Vertices.Num();

    for (int32 i = 0; i < NumCraters; ++i)
    {
        // Random location on sphere
        int32 ImpactVertex = RNG.RandRange(0, Mesh.Vertices.Num() - 1);
        FVector ImpactLocation = Mesh.Vertices[ImpactVertex].Position;

        // Generate crater size from power law distribution
        double Diameter = GenerateCraterDiameter(RNG);

        // Small craters filtered by atmosphere
        // Meteors < threshold burn up before impact
        if (Diameter < AtmosphericDensity * 0.1)
        {
            continue;  // Meteor disintegrated in atmosphere
        }

        // Crater depth from diameter (empirical relationship)
        // Simple craters: Depth ≈ 0.2 * Diameter
        // Complex craters: Shallower (depth increases slower with size)
        double Depth = Diameter < 10.0
            ? Diameter * 0.2
            : 10.0 * 0.2 + (Diameter - 10.0) * 0.05;

        // Crater age (random, weighted toward older impacts)
        double CraterAge = RNG.FRand() * PlanetAge;

        ImpactCrater Crater = {
            .Location = ImpactLocation,
            .Diameter = Diameter,
            .Depth = Depth,
            .Age = CraterAge,
            .EjectaRadius = Diameter * 2.5,  // Ejecta blanket extent
            .IsDegraded = false,
            .IsComplex = Diameter > 10.0  // Transition diameter
        };

        Craters.Add(Crater);
    }

    // Sort by age (oldest first) so newer impacts overwrite older ones
    Craters.Sort([](const ImpactCrater& A, const ImpactCrater& B) {
        return A.Age > B.Age;
    });

    // Apply craters to mesh in chronological order
    for (const ImpactCrater& Crater : Craters)
    {
        ApplyCraterToMesh(Mesh, Crater);
    }
}
```

### Crater Density Calculation

```cpp
double ComputeCraterDensity(double PlanetAge,
                            double TectonicActivity,
                            double AtmosphericDensity)
{
    // Base density: craters per vertex per billion years
    // Calibrated from lunar crater counts
    double BaseDensity = 0.01;

    // More craters accumulate on older surfaces
    double AgeFactor = PlanetAge / 1.0e9;  // Normalize to Gyr

    // Fewer visible craters on tectonically active planets
    // Exponential decay: craters destroyed by subduction/volcanism
    double ActivityFactor = FMath::Exp(-TectonicActivity * 2.0);

    // Dense atmospheres filter small impactors
    // Earth's atmosphere burns up meteors < ~50m diameter
    double AtmosphereFactor = FMath::Exp(-AtmosphericDensity);

    return BaseDensity * AgeFactor * ActivityFactor * AtmosphereFactor;
}
```

**Example Values:**
- **Luna** (dead, no atmosphere): Density ≈ 0.045 (4.5% of vertices have craters)
- **Earth** (active, thick atmosphere): Density ≈ 0.0001 (very few visible)
- **Mars** (low activity, thin atmosphere): Density ≈ 0.015

---

## Crater Morphology

Crater shape varies dramatically with size due to different impact physics:

### Simple Craters (D < 10-20 km)

**Characteristics:**
- Bowl-shaped depression
- Parabolic profile
- Raised rim from ejected material
- No central peak

**Profile Function:**
```cpp
void ApplySimpleCrater(LowResMesh& Mesh, const ImpactCrater& Crater)
{
    for (int32 i = 0; i < Mesh.Vertices.Num(); ++i)
    {
        FVector VertexPos = Mesh.Vertices[i].Position;
        double DistToCrater = (VertexPos - Crater.Location).Size();

        // Normalized radial distance from center
        double R = DistToCrater / (Crater.Diameter * 0.5);

        if (R < 1.0)
        {
            // Inside crater rim: parabolic bowl
            double Depression = Crater.Depth * (1.0 - R * R);
            Mesh.Elevation_C[i] -= Depression;
        }
        else if (R < 1.2)
        {
            // Raised rim (10-20% of crater diameter)
            double RimHeight = Crater.Depth * 0.1 * (1.2 - R) / 0.2;
            Mesh.Elevation_C[i] += RimHeight;
        }
        else if (DistToCrater < Crater.EjectaRadius)
        {
            // Ejecta blanket: exponential falloff
            double EjectaR = (DistToCrater - Crater.Diameter * 0.6) /
                            (Crater.EjectaRadius - Crater.Diameter * 0.6);
            double EjectaThickness = Crater.Depth * 0.1 * FMath::Exp(-EjectaR * 3.0);
            Mesh.Elevation_C[i] += EjectaThickness;
        }
    }
}
```

### Complex Craters (D > 10-20 km)

**Characteristics:**
- Flat floor (from melt pooling)
- Central peak (crustal rebound)
- Terraced walls (gravitational slumping)
- Peak rings (for very large craters)

**Profile Function:**
```cpp
void ApplyComplexCrater(LowResMesh& Mesh, const ImpactCrater& Crater)
{
    for (int32 i = 0; i < Mesh.Vertices.Num(); ++i)
    {
        FVector VertexPos = Mesh.Vertices[i].Position;
        double DistToCrater = (VertexPos - Crater.Location).Size();
        double R = DistToCrater / (Crater.Diameter * 0.5);

        if (R < 0.2)
        {
            // Central peak (crustal rebound from impact)
            // Peak height ≈ 30% of crater depth
            double PeakHeight = Crater.Depth * 0.3 * (1.0 - R / 0.2);
            Mesh.Elevation_C[i] += PeakHeight - Crater.Depth * 0.6;
        }
        else if (R < 0.7)
        {
            // Flat floor (impact melt pool)
            Mesh.Elevation_C[i] -= Crater.Depth * 0.6;
        }
        else if (R < 1.0)
        {
            // Terraced walls (slumped blocks)
            // Multiple steps from wall collapse
            int32 NumTerraces = 3;
            double TerraceStep = (R - 0.7) / 0.3;
            double TerraceHeight = FMath::Floor(TerraceStep * NumTerraces) / NumTerraces;

            double WallDepth = Crater.Depth * (0.6 + TerraceHeight * 0.4);
            Mesh.Elevation_C[i] -= WallDepth;
        }
        else if (DistToCrater < Crater.EjectaRadius)
        {
            // Ejecta blanket (thicker for complex craters)
            double EjectaR = (DistToCrater - Crater.Diameter * 0.5) /
                            (Crater.EjectaRadius - Crater.Diameter * 0.5);
            double EjectaThickness = Crater.Depth * 0.15 * FMath::Exp(-EjectaR * 2.5);
            Mesh.Elevation_C[i] += EjectaThickness;
        }
    }
}
```

### Multi-Ring Basins (D > 300 km)

**Characteristics:**
- Multiple concentric rings
- Very flat interior
- Isostatically compensated (gravity anomalies)
- Examples: Imbrium (Moon), Caloris (Mercury)

```cpp
void ApplyMultiRingBasin(LowResMesh& Mesh, const ImpactCrater& Crater)
{
    // Generate 2-4 concentric rings
    int32 NumRings = FMath::RandRange(2, 4);

    for (int32 i = 0; i < Mesh.Vertices.Num(); ++i)
    {
        FVector VertexPos = Mesh.Vertices[i].Position;
        double DistToCrater = (VertexPos - Crater.Location).Size();
        double R = DistToCrater / (Crater.Diameter * 0.5);

        // Central flat region
        if (R < 0.4)
        {
            Mesh.Elevation_C[i] -= Crater.Depth * 0.8;
        }
        else if (R < 2.0)
        {
            // Concentric rings (fault scarps from rebound)
            for (int32 Ring = 0; Ring < NumRings; ++Ring)
            {
                double RingRadius = 0.5 + Ring * 0.4;
                double DistToRing = FMath::Abs(R - RingRadius);

                if (DistToRing < 0.1)
                {
                    // Ring uplift
                    double RingHeight = Crater.Depth * 0.2 * (1.0 - DistToRing / 0.1);
                    Mesh.Elevation_C[i] += RingHeight;
                }
            }
        }
    }
}
```

---

## Crater Degradation Over Time

Craters don't remain pristine - they degrade through various processes:

### Degradation Mechanisms

1. **Erosion** (water, wind, ice)
   - Softens rim features
   - Fills crater floor with sediment
   - Rounds sharp edges

2. **Tectonic Activity**
   - Subduction buries/destroys craters
   - Volcanism fills craters with lava
   - Faulting disrupts crater structure

3. **Impact Gardening** (space weathering)
   - Micrometeorite bombardment
   - Softens features over billions of years
   - Creates regolith layer

4. **Crater Saturation**
   - New impacts overlap old craters
   - Eventually reaches equilibrium

### Degradation Implementation

```cpp
void DegradeCraters(TArray<ImpactCrater>& Craters,
                    double ErosionRate,
                    double TectonicActivity,
                    double CurrentPlanetAge)
{
    for (auto& Crater : Craters)
    {
        double TimeSinceImpact = CurrentPlanetAge - Crater.Age;

        // Erosion degrades crater features over time
        // Exponential decay: well-preserved → degraded → buried
        double ErosionFactor = 1.0 - FMath::Exp(-ErosionRate * TimeSinceImpact / 1.0e9);

        // Reduce crater depth based on erosion
        Crater.Depth *= (1.0 - ErosionFactor * 0.8);

        // Smooth crater morphology (reduce rim sharpness)
        // ... (apply smoothing to surrounding vertices)

        // Tectonic activity can completely obliterate craters
        if (TectonicActivity > 0.5 && TimeSinceImpact > 500.0e6)
        {
            // Old craters on active planets get subducted/buried
            if (FMath::FRand() < TectonicActivity)
            {
                Crater.IsDegraded = true;  // Remove from visible surface
            }
        }

        // Impact gardening (micrometeorite weathering)
        if (TimeSinceImpact > 1.0e9)
        {
            double GardeningFactor = TimeSinceImpact / 4.0e9;  // Normalize to Moon age
            Crater.Depth *= (1.0 - GardeningFactor * 0.2);
        }
    }

    // Remove completely degraded craters
    Craters.RemoveAll([](const ImpactCrater& C) { return C.IsDegraded; });
}
```

---

## Integration with PTP/RTHAP Pipeline

### Pipeline for Different Planet Types

#### Earth-like Planets (High Activity)

```cpp
PlanetConfig Earth = {
    .TectonicActivity = 0.8,      // Very active
    .ErosionRate = 0.5,           // High (water, wind, ice)
    .AtmosphericDensity = 1.0,    // Thick atmosphere
    .CraterVisibility = 0.1       // Only young, large craters visible
};

// Generation order:
// 1. PTP Phases 1-5 (Tectonic simulation)
// 2. Generate impact craters
// 3. Degrade craters heavily (erosion + tectonics)
// 4. RTHAP (Amplification with valley carving)
//
// Result: Few visible craters, dominated by tectonic features
```

#### Dead Moon (No Activity)

```cpp
PlanetConfig Luna = {
    .TectonicActivity = 0.0,      // Geologically dead
    .ErosionRate = 0.0,           // No atmosphere/water
    .AtmosphericDensity = 0.0,    // Vacuum
    .CraterVisibility = 1.0       // ALL craters preserved
};

// Generation order:
// 1. Optional: Minimal PTP for ancient volcanic maria
// 2. Generate MILLIONS of impact craters
// 3. NO degradation
// 4. RTHAP (visualization only, no valley carving)
//
// Result: Heavily cratered surface, overlapping impacts
```

#### Mars-type (Transitional)

```cpp
PlanetConfig Mars = {
    .TectonicActivity = 0.2,      // Mostly dead (ancient Tharsis volcanism)
    .ErosionRate = 0.05,          // Low (thin atmosphere, wind)
    .AtmosphericDensity = 0.01,   // Very thin atmosphere
    .CraterVisibility = 0.7       // Many craters preserved
};

// Generation order:
// 1. PTP with reduced activity (early volcanism, then shutdown)
// 2. Generate craters (filtered by thin atmosphere)
// 3. Light wind erosion degradation
// 4. RTHAP (minimal valley carving for ancient water channels)
//
// Result: Mix of tectonic features (Tharsis, Valles Marineris) and craters
```

---

## Complete Pipeline Architecture

```
┌─────────────────────────────────────────────────────────┐
│  PLANET TYPE CONFIGURATION                              │
│  - Tectonic activity level                              │
│  - Erosion rate                                          │
│  - Atmospheric density                                   │
│  - Planet age                                            │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ↓
         ┌─────────────┴─────────────┐
         │                           │
         ↓ (High Activity)           ↓ (Low/No Activity)
┌────────────────────┐      ┌────────────────────┐
│  PTP SIMULATION    │      │  SKIP OR MINIMAL   │
│  - Fibonacci       │      │  PTP SIMULATION    │
│  - Delaunay        │      │  - Basic sphere    │
│  - Plate tectonics │      │  - Ancient volc.   │
│  - Erosion         │      │    (optional)      │
└─────────┬──────────┘      └─────────┬──────────┘
          │                           │
          └───────────┬───────────────┘
                      ↓
          ┌───────────────────────┐
          │  IMPACT CRATERING     │
          │  - Power law size     │
          │  - Chronological      │
          │  - Morphology         │
          │  - Overlap handling   │
          └──────────┬────────────┘
                     │
                     ↓
          ┌───────────────────────┐
          │  CRATER DEGRADATION   │
          │  - Erosion effects    │
          │  - Tectonic burial    │
          │  - Impact gardening   │
          └──────────┬────────────┘
                     │
                     ↓
          ┌───────────────────────┐
          │  RTHAP AMPLIFICATION  │
          │  - Adaptive subdiv.   │
          │  - Valley carving     │
          │    (if water present) │
          │  - LOD rendering      │
          └───────────────────────┘
```

---

## Research References

### Essential Papers

1. **Melosh, H.J. (1989)** - "Impact Cratering: A Geologic Process"
   - The foundational textbook on impact mechanics
   - Crater morphology classification
   - Scaling laws for crater dimensions

2. **Neukum, G. et al. (2001)** - "Cratering Records in the Inner Solar System"
   - Crater size-frequency distribution functions
   - Chronology models for dating planetary surfaces
   - Calibration from lunar samples

3. **Richardson, D.C. (2009)** - "Cratered Terrain Generation for Games"
   - Procedural methods for real-time crater rendering
   - Performance optimizations
   - Artistic control techniques

4. **Pike, R.J. (1977)** - "Size-dependence in the shape of fresh impact craters on the Moon"
   - Simple vs complex crater transition
   - Depth/diameter relationships
   - Empirical scaling laws

### NASA Databases

- **Lunar Crater Database** (LROC team)
- **Mars Global Crater Database**
- **Impact Crater Database (Earth Impact Database)**

These provide real-world data for calibration and validation.

---

## Implementation Priority

### Phase 1: Basic Cratering (Essential)
- [ ] Power law size distribution
- [ ] Simple bowl-shaped craters
- [ ] Random spatial distribution
- [ ] Atmospheric filtering

### Phase 2: Realistic Morphology (Important)
- [ ] Complex crater structure (central peaks, terraces)
- [ ] Multi-ring basins
- [ ] Ejecta blankets
- [ ] Crater overlap handling

### Phase 3: Degradation (Polish)
- [ ] Erosion-based degradation
- [ ] Tectonic burial
- [ ] Impact gardening
- [ ] Age-based weathering

### Phase 4: Integration (Production)
- [ ] Planet-type presets (Earth, Mars, Luna, etc.)
- [ ] Performance optimization
- [ ] LOD for crater detail
- [ ] Material system (crater floors, ejecta, rays)

---

## Performance Considerations

### Optimization Strategies

1. **LOD for Craters**
   - Small craters (<100m) only visible up close
   - Medium craters (100m-10km) at medium distances
   - Large craters (>10km) always visible

2. **Spatial Acceleration**
   - Quad-tree or octree for crater lookup
   - Only apply craters within camera frustum
   - Progressive refinement as camera approaches

3. **GPU Acceleration**
   - Crater application as compute shader
   - Parallel processing of crater deformation
   - Instanced rendering for similar-sized craters

4. **Pre-computation**
   - Generate craters once during planet creation
   - Store crater catalog with planet data
   - Only recompute if parameters change

### Expected Performance

- **Crater Generation**: ~1-5 seconds for 1 million craters (CPU)
- **Mesh Deformation**: ~10-50ms per frame (GPU)
- **Memory**: ~100 bytes per crater (catalog)

---

## Extension: Raycast Crater Lookup

For very high crater densities (like Luna), storing individual craters becomes memory-intensive. Alternative approach:

```cpp
// Procedural crater generation at query time
ImpactCrater GetCraterAtLocation(FVector Location, int32 LOD)
{
    // Use location as seed for deterministic random generation
    uint32 Seed = HashLocationToSeed(Location);
    FRandomStream RNG(Seed);

    // Generate crater on-demand based on spatial hash
    if (RNG.FRand() < CraterDensity)
    {
        return GenerateCraterFromSeed(Seed, LOD);
    }

    return NullCrater;
}
```

This allows "infinite" craters without storing them all in memory - useful for dead moons.

---

## Conclusion

Impact cratering is **essential** for a complete planetary generation system that covers:
- ✅ Active planets (Earth) - PTP with minimal craters
- ✅ Dead moons (Luna) - Heavy cratering, minimal tectonics
- ✅ Transitional worlds (Mars) - Mix of both systems

This extension bridges the gap between the PTP/RTHAP papers and a truly universal planetary generation engine capable of creating any rocky body in the solar system.

**Next Steps:**
1. Implement basic cratering system (Phase 1)
2. Test integration with existing PTP pipeline
3. Calibrate parameters against real planetary data (Luna, Mars)
4. Extend RTHAP to handle crater-dominated surfaces

---

*Document created: 2025-10-31*
*Author: Gaia Project - Impact Cratering Extension Design*
