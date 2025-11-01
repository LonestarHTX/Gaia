// Copyright (c) Gaia project
using UnrealBuildTool;

public class GaiaPTP : ModuleRules
{
    public GaiaPTP(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "RealtimeMeshComponent",
            "GeometryCore",
            "GeometryFramework",
            "DeveloperSettings"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects",
            // Access public headers from CGAL provider module (IPTPAdjacencyProvider)
            "GaiaPTPCGAL"
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
