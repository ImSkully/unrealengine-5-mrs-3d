#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BitmapPoint.generated.h"

/**
 * Represents a 3D point with bitmap data from AR/MR systems
 */
USTRUCT(BlueprintType)
struct FMRS3DPLUGIN_API FBitmapPoint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D")
	FVector Position;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D")
	FColor Color;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D")
	float Intensity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D")
	float Timestamp;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D")
	FVector Normal;

	FBitmapPoint()
		: Position(FVector::ZeroVector)
		, Color(FColor::White)
		, Intensity(1.0f)
		, Timestamp(FPlatformTime::Seconds())
		, Normal(FVector::UpVector)
	{}

	FBitmapPoint(const FVector& InPosition, const FColor& InColor, float InIntensity = 1.0f)
		: Position(InPosition)
		, Color(InColor)
		, Intensity(InIntensity)
		, Timestamp(FPlatformTime::Seconds())
		, Normal(FVector::UpVector)
	{}
};
