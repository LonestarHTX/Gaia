#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogGaiaPTPEditor, Log, All);

class FGaiaPTPEditorModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogGaiaPTPEditor, Log, TEXT("GaiaPTP editor module starting"));
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogGaiaPTPEditor, Log, TEXT("GaiaPTP editor module shutting down"));
    }
};

IMPLEMENT_MODULE(FGaiaPTPEditorModule, GaiaPTPEditor)
