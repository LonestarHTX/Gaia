#include "GaiaPTP.h"
#include "Modules/ModuleManager.h"
#include "PTPProfiling.h"

DEFINE_LOG_CATEGORY(LogGaiaPTP);

class FGaiaPTPModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogGaiaPTP, Log, TEXT("GaiaPTP runtime module starting"));
        PTPProfiling::RegisterConsoleCommands();
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogGaiaPTP, Log, TEXT("GaiaPTP runtime module shutting down"));
        PTPProfiling::UnregisterConsoleCommands();
    }
};

IMPLEMENT_MODULE(FGaiaPTPModule, GaiaPTP)
