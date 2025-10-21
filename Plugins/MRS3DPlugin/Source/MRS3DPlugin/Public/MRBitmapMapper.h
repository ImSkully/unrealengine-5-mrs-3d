#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BitmapPoint.h"
#include "PlaneDetection.h"
#include "MRBitmapMapper.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBitmapPointsUpdated, const TArray<FBitmapPoint>&, BitmapPoints);

/**
 * Subsystem for mapping AR/MR bitmap points into the engine in real-time
 */
UCLASS()
class FMRS3DPLUGIN_API UMRBitmapMapper : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UMRBitmapMapper();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Add a new bitmap point from AR/MR system
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	void AddBitmapPoint(const FBitmapPoint& Point);

	/**
	 * Add multiple bitmap points at once
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	void AddBitmapPoints(const TArray<FBitmapPoint>& Points);

	/**
	 * Clear all bitmap points
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	void ClearBitmapPoints();

	/**
	 * Get all current bitmap points
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	const TArray<FBitmapPoint>& GetBitmapPoints() const { return BitmapPoints; }

	/**
	 * Get bitmap points within a specified radius
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	TArray<FBitmapPoint> GetBitmapPointsInRadius(const FVector& Center, float Radius) const;

	/**
	 * Enable or disable real-time updates
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	void SetRealTimeUpdates(bool bEnabled);

	/**
	 * Set maximum number of bitmap points to store (0 = unlimited)
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	void SetMaxBitmapPoints(int32 MaxPoints);

	/**
	 * Set maximum age in seconds for bitmap points (0 = no age limit)
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	void SetMaxPointAge(float MaxAgeSeconds);

	/**
	 * Remove old bitmap points based on age
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	int32 RemoveOldPoints();

	/**
	 * Get current memory usage statistics
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	int32 GetMemoryUsageKB() const;

	/**
	 * Force garbage collection of old points
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	void ForceCleanup();

	/**
	 * Enable/disable automatic plane detection from point clouds
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	void SetAutoPlaneDetectionEnabled(bool bEnabled);

	/**
	 * Manually trigger plane detection from current points
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	TArray<FDetectedPlane> DetectPlanesFromCurrentPoints(float PlaneThickness = 0.1f);

	/**
	 * Update AR/MR tracking state
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	void UpdateARTrackingState(ETrackingState NewState, float Quality = 1.0f, const FString& LossReason = TEXT(""));

	/**
	 * Get current AR/MR tracking state
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	ETrackingState GetCurrentTrackingState() const;

	/**
	 * Event fired when bitmap points are updated
	 */
	UPROPERTY(BlueprintAssignable, Category = "MRS3D|MR")
	FOnBitmapPointsUpdated OnBitmapPointsUpdated;

protected:
	UPROPERTY()
	TArray<FBitmapPoint> BitmapPoints;

	UPROPERTY()
	bool bRealTimeUpdatesEnabled;

	// Memory management settings
	UPROPERTY(Config)
	int32 MaxBitmapPoints;

	UPROPERTY(Config)
	float MaxPointAgeSeconds;

	UPROPERTY(Config)
	bool bAutoCleanupEnabled;

	UPROPERTY(Config)
	float CleanupIntervalSeconds;

	// Internal cleanup tracking
	float LastCleanupTime;

	// Plane detection settings
	UPROPERTY(Config)
	bool bAutoPlaneDetectionEnabled;

	UPROPERTY(Config)
	float PlaneDetectionInterval;

	UPROPERTY(Config)
	int32 MinPointsForPlaneDetection;

	// Internal plane detection tracking
	float LastPlaneDetectionTime;
	ETrackingState CurrentTrackingState;

	void BroadcastUpdate();
	void PerformAutoCleanup();
	void RemoveExcessPoints();
	void RemovePointsByAge(float CurrentTime);
	void PerformAutoPlaneDetection();
};
