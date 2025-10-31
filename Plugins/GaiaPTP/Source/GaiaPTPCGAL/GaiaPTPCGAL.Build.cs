using UnrealBuildTool;
using System;
using System.IO;

public class GaiaPTPCGAL : ModuleRules
{
    public GaiaPTPCGAL(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GaiaPTP"
        });

        // Detect vcpkg install for CGAL headers
        // ThirdParty wrapper include
        string tpInclude = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/PTP_CGAL/include"));
        if (Directory.Exists(tpInclude))
        {
            PublicIncludePaths.Add(tpInclude);
        }

        // Find prebuilt static lib
        bool bWithPtpCgalLib = false;
        string tpBuild = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/PTP_CGAL/build"));
        if (Directory.Exists(tpBuild))
        {
            foreach (var lib in Directory.GetFiles(tpBuild, "ptp_cgal.lib", SearchOption.AllDirectories))
            {
                PublicAdditionalLibraries.Add(lib);
                bWithPtpCgalLib = true;
                break;
            }
        }

        // Link required vcpkg libs (GMP/MPFR) if available
        string vcpkgRoot = Environment.GetEnvironmentVariable("VCPKG_ROOT");
        if (string.IsNullOrEmpty(vcpkgRoot))
        {
            string userRoot = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
            string candidate = Path.Combine(userRoot, "vcpkg");
            if (Directory.Exists(candidate)) vcpkgRoot = candidate;
        }
        if (!string.IsNullOrEmpty(vcpkgRoot))
        {
            string libDir = Path.Combine(vcpkgRoot, "installed", "x64-windows", "lib");
            if (Directory.Exists(libDir))
            {
                PublicAdditionalLibraries.Add(Path.Combine(libDir, "mpfr.lib"));
                PublicAdditionalLibraries.Add(Path.Combine(libDir, "gmp.lib"));
            }

            string binDir = Path.Combine(vcpkgRoot, "installed", "x64-windows", "bin");
            if (Directory.Exists(binDir))
            {
                string[] dlls = new[] { "gmp-10.dll", "gmpxx-4.dll", "mpfr-6.dll" };
                foreach (var dll in dlls)
                {
                    string src = Path.Combine(binDir, dll);
                    if (File.Exists(src))
                    {
                        PublicDelayLoadDLLs.Add(dll);
                        RuntimeDependencies.Add("$(TargetOutputDir)/" + dll, src);
                    }
                }
            }
        }

        PublicDefinitions.Add(bWithPtpCgalLib ? "WITH_PTP_CGAL_LIB=1" : "WITH_PTP_CGAL_LIB=0");
    }
}
