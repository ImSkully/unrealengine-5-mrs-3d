#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PlaneDetection.h"
#include "BitmapPoint.h"
#include "PlaneDetectionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlaneDetected, const FDetectedPlane&, DetectedPlane);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlaneUpdated, const FDetectedPlane&, UpdatedPlane);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlaneLost, const FString&, PlaneID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrackingStateChanged, ETrackingState, NewState);

/**
 * Subsystem for plane detection and tracking in AR/MR environments
 */
UCLASS()
class FMRS3DPLUGIN_API UPlaneDetectionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPlaneDetectionSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Add a detected plane to the system
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	void AddDetectedPlane(const FDetectedPlane& Plane);

	/**
	 * Update an existing plane
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	bool UpdatePlane(const FString& PlaneID, const FDetectedPlane& UpdatedPlane);

	/**
	 * Remove a plane by ID
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	bool RemovePlane(const FString& PlaneID);

	/**
	 * Get all detected planes
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	TArray<FDetectedPlane> GetAllPlanes() const;

	/**
	 * Get planes by type
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	TArray<FDetectedPlane> GetPlanesByType(EPlaneType PlaneType) const;

	/**
	 * Get the largest plane of a specific type
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	FDetectedPlane GetLargestPlane(EPlaneType PlaneType = EPlaneType::Unknown) const;

	/**
	 * Find planes within a radius of a point
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	TArray<FDetectedPlane> GetPlanesInRadius(const FVector& Center, float Radius) const;

	/**
	 * Project a point onto the nearest plane
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	FVector ProjectPointToNearestPlane(const FVector& Point, EPlaneType PlaneType = EPlaneType::Unknown) const;

	/**
	 * Update tracking state
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	void UpdateTrackingState(ETrackingState NewState, float Quality = 1.0f, const FString& LossReason = TEXT(""));

	/**
	 * Get current tracking session
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	FTrackingSession GetTrackingSession() const { return CurrentTrackingSession; }

	/**
	 * Clear all detected planes
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	void ClearAllPlanes();

	/**
	 * Enable/disable plane detection
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	void SetPlaneDetectionEnabled(bool bEnabled);

	/**
	 * Set minimum plane size for detection
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	void SetMinimumPlaneSize(float MinArea);

	/**
	 * Detect planes from point cloud data
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	TArray<FDetectedPlane> DetectPlanesFromPoints(const TArray<FBitmapPoint>& Points, float PlaneThickness = 0.1f);

	/**
	 * Validate plane tracking (remove stale planes)
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|PlaneDetection")
	int32 ValidatePlaneTracking(float MaxAge = 30.0f);

	// Events
	UPROPERTY(BlueprintAssignable, Category = "MRS3D|PlaneDetection")
	FOnPlaneDetected OnPlaneDetected;

	UPROPERTY(BlueprintAssignable, Category = "MRS3D|PlaneDetection")
	FOnPlaneUpdated OnPlaneUpdated;

	UPROPERTY(BlueprintAssignable, Category = "MRS3D|PlaneDetection")
	FOnPlaneLost OnPlaneLost;

	UPROPERTY(BlueprintAssignable, Category = "MRS3D|PlaneDetection")
	FOnTrackingStateChanged OnTrackingStateChanged;

protected:
	/** Map of detected planes by ID */
	UPROPERTY()
	TMap<FString, FDetectedPlane> DetectedPlanes;

	/** Current tracking session */
	UPROPERTY()
	FTrackingSession CurrentTrackingSession;

	/** Configuration settings */
	UPROPERTY(Config)
	bool bPlaneDetectionEnabled;

	UPROPERTY(Config)
	float MinimumPlaneArea;

	UPROPERTY(Config)
	float PlaneValidationInterval;

	UPROPERTY(Config)
	float MaxPlaneAge;

	UPROPERTY(Config)
	bool bAutoValidatePlanes;

	/** Internal tracking */
	float LastValidationTime;

	/** Helper methods */
	void PerformAutoValidation();
	EPlaneType ClassifyPlane(const FVector& Normal, const FVector& Center) const;
	FString GeneratePlaneID() const;
	bool IsPlaneValid(const FDetectedPlane& Plane) const;
};