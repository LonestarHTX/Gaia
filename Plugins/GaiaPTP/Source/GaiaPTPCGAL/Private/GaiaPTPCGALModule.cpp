#include "Modules/ModuleManager.h"

class FGaiaPTPCGALModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FGaiaPTPCGALModule, GaiaPTPCGAL)

