using UnrealBuildTool;

public class GaiaPTPEditor : ModuleRules
{
    public GaiaPTPEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "Slate",
            "SlateCore",
            "GaiaPTP",
            "RealtimeMeshComponent"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects"
        });

        PublicIncludePaths.AddRange(new string[]
        {
            System.IO.Path.Combine(ModuleDirectory, "Public")
        });
        PrivateIncludePaths.AddRange(new string[]
        {
            System.IO.Path.Combine(ModuleDirectory, "Private")
        });
    }
}
