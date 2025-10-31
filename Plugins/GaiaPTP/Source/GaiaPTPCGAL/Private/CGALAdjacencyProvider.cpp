#include "IPTPAdjacencyProvider.h"

#if WITH_PTP_CGAL_LIB

#include "ptp_cgal.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DEFINE_CATEGORY(GAIA_PTP_CGAL, true);
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

class FCGALAdjacencyProvider final : public IPTPAdjacencyProvider
{
public:
    virtual bool Build(const TArray<FVector>& InPoints, FPTPAdjacency& OutAdj, FString& OutError) override
    {
        if (InPoints.Num() < 4)
        {
            OutError = TEXT("Insufficient points for triangulation");
            return false;
        }

        // Prepare input buffer
        TArray<double> XYZ; XYZ.SetNumUninitialized(InPoints.Num()*3);
        for (int32 i=0;i<InPoints.Num();++i)
        {
            XYZ[3*i+0] = InPoints[i].X;
            XYZ[3*i+1] = InPoints[i].Y;
            XYZ[3*i+2] = InPoints[i].Z;
        }

        // Ensure dependent DLLs are loaded (gmp/mpfr via vcpkg)
        {
            auto TryLoad = [](const FString& DllPath)
            {
                if (DllPath.IsEmpty()) return (void*)nullptr;
                return FPlatformProcess::GetDllHandle(*DllPath);
            };
            // Try plain names first (PATH/module dir)
            void* H1 = FPlatformProcess::GetDllHandle(TEXT("gmp-10.dll"));
            void* H2 = FPlatformProcess::GetDllHandle(TEXT("gmpxx-4.dll"));
            void* H3 = FPlatformProcess::GetDllHandle(TEXT("mpfr-6.dll"));
            if (!H1 || !H2 || !H3)
            {
                TCHAR Buffer[1024];
                FPlatformMisc::GetEnvironmentVariable(TEXT("VCPKG_ROOT"), Buffer, 1024);
                FString Root(Buffer);
                if (!Root.IsEmpty())
                {
                    const FString Bin = FPaths::Combine(Root, TEXT("installed/x64-windows/bin"));
                    if (!H1) H1 = TryLoad(FPaths::Combine(Bin, TEXT("gmp-10.dll")));
                    if (!H2) H2 = TryLoad(FPaths::Combine(Bin, TEXT("gmpxx-4.dll")));
                    if (!H3) H3 = TryLoad(FPaths::Combine(Bin, TEXT("mpfr-6.dll")));
                }
            }
        }

        // Call external library
        const int32 MaxTris = InPoints.Num()*2; // generous capacity
        TArray<int32> Tris; Tris.SetNumUninitialized(MaxTris*3);
        int written = 0;
        int ok = 0; {
            CSV_SCOPED_TIMING_STAT(GAIA_PTP_CGAL, Adjacency);
            ok = ptp_cgal_triangulate(XYZ.GetData(), InPoints.Num(), Tris.GetData(), MaxTris, &written);
        }
        if (!ok || written <= 0)
        {
            OutError = TEXT("ptp_cgal_triangulate failed");
            return false;
        }

        // Fill triangles and compute neighbor adjacency
        OutAdj.Triangles.Reset(written);
        OutAdj.Neighbors.Reset(InPoints.Num());
        OutAdj.Neighbors.SetNum(InPoints.Num());
        TArray<TSet<int32>> Nbs; Nbs.SetNum(InPoints.Num());
        for (int i=0;i<written;++i)
        {
            const int32 a = Tris[3*i+0];
            const int32 b = Tris[3*i+1];
            const int32 c = Tris[3*i+2];
            OutAdj.Triangles.Add(FIntVector(a,b,c));
            Nbs[a].Add(b); Nbs[b].Add(a);
            Nbs[b].Add(c); Nbs[c].Add(b);
            Nbs[c].Add(a); Nbs[a].Add(c);
        }
        for (int32 i=0;i<InPoints.Num();++i)
        {
            Nbs[i].Remove(i);
            TArray<int32>& Arr = OutAdj.Neighbors[i];
            Arr.Empty(Nbs[i].Num());
            for (int32 v : Nbs[i]) { Arr.Add(v); }
        }
        return true;
    }
};

#else

class FCGALAdjacencyProvider final : public IPTPAdjacencyProvider
{
public:
    virtual bool Build(const TArray<FVector>& /*Points*/, FPTPAdjacency& /*OutAdj*/, FString& OutError) override
    {
        OutError = TEXT("CGAL not available (WITH_CGAL=0). Configure vcpkg and rebuild.");
        return false;
    }
};

#endif

TSharedPtr<IPTPAdjacencyProvider> CreateCGALAdjacencyProvider()
{
    return MakeShared<FCGALAdjacencyProvider>();
}
