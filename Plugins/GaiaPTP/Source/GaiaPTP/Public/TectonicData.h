#pragma once

#include "CoreMinimal.h"
#include "TectonicTypes.h"
#include "TectonicData.generated.h"

/** Per-sample crust data stored on the planetary sphere. Units in km unless noted. */
USTRUCT(BlueprintType)
struct GAIAPTP_API FCrustData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Crust")
    ECrustType Type;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Crust")
    float Thickness; // km

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Crust")
    float Elevation; // km relative to sea level

    // Oceanic parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Oceanic")
    float OceanicAge; // My

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Oceanic")
    FVector RidgeDirection; // normalized

    // Continental parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Continental")
    float OrogenyAge; // My

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Continental")
    EOrogenyType OrogenyType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Continental")
    FVector FoldDirection; // normalized

    FCrustData()
        : Type(ECrustType::Oceanic)
        , Thickness(7.0f)
        , Elevation(-6.0f)
        , OceanicAge(0.0f)
        , RidgeDirection(FVector::ZeroVector)
        , OrogenyAge(0.0f)
        , OrogenyType(EOrogenyType::None)
        , FoldDirection(FVector::ZeroVector)
    {}
};

/** Connected region of continental crust that may detach/attach during collisions. */
USTRUCT(BlueprintType)
struct GAIAPTP_API FTerrane
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Terrane")
    int32 TerraneId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Terrane")
    TArray<int32> PointIndices;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Terrane")
    FVector Centroid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Terrane")
    float Area; // km^2

    FTerrane()
        : TerraneId(-1)
        , Centroid(FVector::ZeroVector)
        , Area(0.0f)
    {}
};

/** Plate definition using Euler pole rotation for geodetic motion. */
USTRUCT(BlueprintType)
struct GAIAPTP_API FTectonicPlate
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Plate")
    int32 PlateId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Plate")
    TArray<int32> PointIndices;

    /** Unit vector on sphere indicating the plate seed/centroid position. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Plate")
    FVector CentroidDir;

    // Rotation axis is normalized and passes through origin.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Motion")
    FVector RotationAxis;

    // Angular velocity in radians per My.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Motion")
    float AngularVelocity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PTP|Terranes")
    TArray<FTerrane> Terranes;

    FTectonicPlate()
        : PlateId(-1)
        , RotationAxis(FVector::UpVector)
        , AngularVelocity(0.0f)
    {}

    FORCEINLINE FVector GetAngularVelocityVector() const
    {
        return RotationAxis * AngularVelocity;
    }

    FORCEINLINE FVector GetVelocityAtPoint(const FVector& Point) const
    {
        return FVector::CrossProduct(GetAngularVelocityVector(), Point);
    }
};
