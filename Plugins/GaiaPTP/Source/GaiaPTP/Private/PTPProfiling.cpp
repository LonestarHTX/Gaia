#include "PTPProfiling.h"
#include "HAL/IConsoleManager.h"
#include "GaiaPTP.h"
#include "ProfilingDebugging/CsvProfiler.h"

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

    FAutoConsoleCommand CmdProfileStart(
        TEXT("ptp.profile.start"),
        TEXT("Begin CSV profiling capture for PTP category"),
        FConsoleCommandDelegate::CreateStatic(&PTPProfileStart));

    FAutoConsoleCommand CmdProfileStop(
        TEXT("ptp.profile.stop"),
        TEXT("End CSV profiling capture"),
        FConsoleCommandDelegate::CreateStatic(&PTPProfileStop));
}

namespace PTPProfiling
{
    void RegisterConsoleCommands() {}
    void UnregisterConsoleCommands() {}
}
