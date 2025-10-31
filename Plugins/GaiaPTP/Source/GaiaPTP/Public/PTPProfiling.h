#pragma once

#include "CoreMinimal.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DECLARE_CATEGORY_EXTERN(GAIA_PTP);

namespace PTPProfiling
{
    void RegisterConsoleCommands();
    void UnregisterConsoleCommands();
}

