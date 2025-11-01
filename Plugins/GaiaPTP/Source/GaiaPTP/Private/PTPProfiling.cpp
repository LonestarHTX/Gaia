#include "PTPProfiling.h"
#include "HAL/IConsoleManager.h"
#include "GaiaPTP.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "PTPPlanetComponent.h"

CSV_DEFINE_CATEGORY(GAIA_PTP, true);

namespace
{
    void PTPProfileStart()
    {
        if (!FCsvProfiler::Get()->IsCapturing())
        {
            FCsvProfiler::Get()->BeginCapture();
            UE_LOG(LogGaiaPTP, Log, TEXT("PTP CSV capture started"));
        }
    }
    void PTPProfileStop()
    {
        if (FCsvProfiler::Get()->IsCapturing())
        {
            FCsvProfiler::Get()->EndCapture();
            UE_LOG(LogGaiaPTP, Log, TEXT("PTP CSV capture stopped"));
        }
    }

    // CVar to control parallelization in init paths
    static TAutoConsoleVariable<int32> CVarPTPParallel(
        TEXT("ptp.parallel"),
        1,
        TEXT("Enable (1) or disable (0) ParallelFor in PTP initialization"),
        ECVF_Default);

    FAutoConsoleCommand CmdProfileStart(
        TEXT("ptp.profile.start"),
        TEXT("Begin CSV profiling capture for PTP category"),
        FConsoleCommandDelegate::CreateStatic(&PTPProfileStart));

    FAutoConsoleCommand CmdProfileStop(
        TEXT("ptp.profile.stop"),
        TEXT("End CSV profiling capture"),
        FConsoleCommandDelegate::CreateStatic(&PTPProfileStop));

    // Simple benchmark: rebuilds a transient component using current project settings.
    // Usage: ptp.bench.rebuild
    void PTPBenchRebuild()
    {
        UPTPPlanetComponent* Comp = NewObject<UPTPPlanetComponent>();
        if (!Comp) return;
        Comp->ApplyDefaultsFromProjectSettings();
        Comp->RebuildPlanet();
    }

    FAutoConsoleCommand CmdBenchRebuild(
        TEXT("ptp.bench.rebuild"),
        TEXT("Rebuilds a PTP planet component with current settings to benchmark Phase 1 init"),
        FConsoleCommandDelegate::CreateStatic(&PTPBenchRebuild));

    // Bench CVars
    static TAutoConsoleVariable<int32> CVarPTPBenchNumPoints(
        TEXT("ptp.bench.numPoints"), 100000,
        TEXT("Number of sample points to use for ptp.bench.rebuild_np"), ECVF_Default);
    static TAutoConsoleVariable<int32> CVarPTPBenchNumPlates(
        TEXT("ptp.bench.numPlates"), 40,
        TEXT("Number of plates to use for ptp.bench.rebuild_np"), ECVF_Default);

    void PTPBenchRebuildNP()
    {
        const int32 NumPoints = CVarPTPBenchNumPoints.GetValueOnAnyThread();
        const int32 NumPlates = CVarPTPBenchNumPlates.GetValueOnAnyThread();
        UPTPPlanetComponent* Comp = NewObject<UPTPPlanetComponent>();
        if (!Comp) return;
        Comp->ApplyDefaultsFromProjectSettings();
        Comp->NumSamplePoints = NumPoints;
        Comp->NumPlates = NumPlates;
        Comp->RebuildPlanet();
    }

    FAutoConsoleCommand CmdBenchRebuildNP(
        TEXT("ptp.bench.rebuild_np"),
        TEXT("Rebuilds a PTP planet with NumSamplePoints=ptp.bench.numPoints and NumPlates=ptp.bench.numPlates"),
        FConsoleCommandDelegate::CreateStatic(&PTPBenchRebuildNP));
}

namespace PTPProfiling
{
    void RegisterConsoleCommands() {}
    void UnregisterConsoleCommands() {}
}
