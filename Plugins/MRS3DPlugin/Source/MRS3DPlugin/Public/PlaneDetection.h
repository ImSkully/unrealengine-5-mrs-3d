#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PlaneDetection.generated.h"

/**
 * Tracking state for AR/MR systems
 */
UENUM(BlueprintType)
enum class ETrackingState : uint8
{
	NotTracking UMETA(DisplayName = "Not Tracking"),
	LimitedTracking UMETA(DisplayName = "Limited Tracking"),
	FullTracking UMETA(DisplayName = "Full Tracking"),
	TrackingLost UMETA(DisplayName = "Tracking Lost")
};

/**
 * Types of detected planes
 */
UENUM(BlueprintType)
enum class EPlaneType : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),
	Horizontal UMETA(DisplayName = "Horizontal"),
	Vertical UMETA(DisplayName = "Vertical"),
	Angled UMETA(DisplayName = "Angled"),
	Floor UMETA(DisplayName = "Floor"),
	Wall UMETA(DisplayName = "Wall"),
	Ceiling UMETA(DisplayName = "Ceiling"),
	Table UMETA(DisplayName = "Table")
};

/**
 * Plane detection confidence levels
 */
UENUM(BlueprintType)
enum class EPlaneConfidence : uint8
{
	Low UMETA(DisplayName = "Low"),
	Medium UMETA(DisplayName = "Medium"),
	High UMETA(DisplayName = "High")
};

/**
 * Represents a detected plane in AR/MR space
 */
USTRUCT(BlueprintType)
struct FMRS3DPLUGIN_API FDetectedPlane
{
	GENERATED_BODY()

	/** Unique identifier for this plane */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Plane")
	FString PlaneID;

	/** Center position of the plane */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Plane")
	FVector Center;

	/** Normal vector of the plane */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Plane")
	FVector Normal;

	/** Extent (half-sizes) of the plane */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Plane")
	FVector2D Extent;

	/** Type of the detected plane */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Plane")
	EPlaneType PlaneType;

	/** Confidence level of the detection */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Plane")
	EPlaneConfidence Confidence;

	/** Timestamp when plane was detected */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Plane")
	float DetectionTime;

	/** Last update timestamp */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Plane")
	float LastUpdateTime;

	/** Whether this plane is currently being tracked */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Plane")
	bool bIsTracked;

	/** Area of the plane in square units */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Plane")
	float Area;

	/** Boundary points of the plane (optional) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Plane")
	TArray<FVector> BoundaryPoints;

	FDetectedPlane()
		: PlaneID(TEXT(""))
		, Center(FVector::ZeroVector)
		, Normal(FVector::UpVector)
		, Extent(FVector2D::ZeroVector)
		, PlaneType(EPlaneType::Unknown)
		, Confidence(EPlaneConfidence::Low)
		, DetectionTime(FPlatformTime::Seconds())
		, LastUpdateTime(FPlatformTime::Seconds())
		, bIsTracked(false)
		, Area(0.0f)
	{}

	FDetectedPlane(const FString& InPlaneID, const FVector& InCenter, const FVector& InNormal, const FVector2D& InExtent)
		: PlaneID(InPlaneID)
		, Center(InCenter)
		, Normal(InNormal)
		, Extent(InExtent)
		, PlaneType(EPlaneType::Unknown)
		, Confidence(EPlaneConfidence::Medium)
		, DetectionTime(FPlatformTime::Seconds())
		, LastUpdateTime(FPlatformTime::Seconds())
		, bIsTracked(true)
		, Area(InExtent.X * InExtent.Y * 4.0f) // 4 * half-width * half-height
	{}

	/** Calculate the area of the plane */
	float GetArea() const
	{
		return Extent.X * Extent.Y * 4.0f;
	}

	/** Check if the plane is horizontal (floor/ceiling) */
	bool IsHorizontal() const
	{
		return PlaneType == EPlaneType::Horizontal || 
			   PlaneType == EPlaneType::Floor || 
			   PlaneType == EPlaneType::Ceiling ||
			   PlaneType == EPlaneType::Table;
	}

	/** Check if the plane is vertical (wall) */
	bool IsVertical() const
	{
		return PlaneType == EPlaneType::Vertical || 
			   PlaneType == EPlaneType::Wall;
	}

	/** Update the plane's timestamp */
	void UpdateTimestamp()
	{
		LastUpdateTime = FPlatformTime::Seconds();
	}
};

/**
 * Tracking session information
 */
USTRUCT(BlueprintType)
struct FMRS3DPLUGIN_API FTrackingSession
{
	GENERATED_BODY()

	/** Current tracking state */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Tracking")
	ETrackingState CurrentState;

	/** Tracking quality (0.0 to 1.0) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Tracking")
	float TrackingQuality;

	/** Time when tracking started */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Tracking")
	float SessionStartTime;

	/** Last tracking update time */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Tracking")
	float LastUpdateTime;

	/** Reason for tracking loss (if applicable) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Tracking")
	FString TrackingLossReason;

	/** Whether the tracking session is active */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Tracking")
	bool bIsActive;

	/** Number of tracking interruptions */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MRS3D|Tracking")
	int32 TrackingInterruptions;

	FTrackingSession()
		: CurrentState(ETrackingState::NotTracking)
		, TrackingQuality(0.0f)
		, SessionStartTime(FPlatformTime::Seconds())
		, LastUpdateTime(FPlatformTime::Seconds())
		, TrackingLossReason(TEXT(""))
		, bIsActive(false)
		, TrackingInterruptions(0)
	{}

	/** Update tracking state */
	void UpdateState(ETrackingState NewState, float Quality = 1.0f)
	{
		if (CurrentState != NewState)
		{
			if (NewState == ETrackingState::TrackingLost || NewState == ETrackingState::NotTracking)
			{
				TrackingInterruptions++;
			}
			CurrentState = NewState;
		}
		
		TrackingQuality = FMath::Clamp(Quality, 0.0f, 1.0f);
		LastUpdateTime = FPlatformTime::Seconds();
		bIsActive = (NewState == ETrackingState::FullTracking || NewState == ETrackingState::LimitedTracking);
	}

	/** Get session duration */
	float GetSessionDuration() const
	{
		return FPlatformTime::Seconds() - SessionStartTime;
	}

	/** Check if tracking is reliable */
	bool IsTrackingReliable() const
	{
		return CurrentState == ETrackingState::FullTracking && TrackingQuality > 0.7f;
	}
};