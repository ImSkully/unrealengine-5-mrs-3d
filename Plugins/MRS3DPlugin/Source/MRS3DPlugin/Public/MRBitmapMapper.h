#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BitmapPoint.h"
#include "PlaneDetection.h"
#include "BitmapPointStorage.h"
#include "BitmapPointMemoryManager.h"
#include "BitmapPointSpatialIndex.h"
#include "MRTrackingStateManager.h"
#include "MRBitmapMapper.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBitmapPointsUpdated, const TArray<FBitmapPoint>&, BitmapPoints);

/**
 * Refactored subsystem for mapping AR/MR bitmap points using specialized components
 * Delegates responsibilities to focused components following Single Responsibility Principle
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
	const TArray<FBitmapPoint>& GetBitmapPoints() const;

	/**
	 * Get bitmap points within a specified radius
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	TArray<FBitmapPoint> GetBitmapPointsInRadius(const FVector& Center, float Radius) const;

	/**
	 * Find the nearest point to a location
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	bool FindNearestPoint(const FVector& Location, FBitmapPoint& OutPoint, float MaxDistance = 1000.0f) const;

	/**
	 * Find K nearest points to a location
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	TArray<FBitmapPoint> FindKNearestPoints(const FVector& Location, int32 K, float MaxDistance = 1000.0f) const;

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
	void UpdateARTrackingState(ETrackingState NewState, const FTransform& CameraPose, float Quality = 1.0f);

	/**
	 * Get current AR/MR tracking state
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	ETrackingState GetCurrentTrackingState() const;

	/**
	 * Get tracking session information
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	FMRSessionInfo GetTrackingSessionInfo() const;

	/**
	 * Check if tracking is currently stable
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR")
	bool IsTrackingStable() const;

	/**
	 * Get specialized component instances for advanced usage
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR|Advanced")
	UBitmapPointStorage* GetStorageComponent() const { return Storage; }

	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR|Advanced")
	UBitmapPointMemoryManager* GetMemoryManager() const { return MemoryManager; }

	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR|Advanced")
	UBitmapPointSpatialIndex* GetSpatialIndex() const { return SpatialIndex; }

	UFUNCTION(BlueprintCallable, Category = "MRS3D|MR|Advanced")
	UMRTrackingStateManager* GetTrackingStateManager() const { return TrackingStateManager; }

	/**
	 * Event fired when bitmap points are updated
	 */
	UPROPERTY(BlueprintAssignable, Category = "MRS3D|MR")
	FOnBitmapPointsUpdated OnBitmapPointsUpdated;
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
	/** Specialized components following Single Responsibility Principle */
	
	/** Pure data storage component */
	UPROPERTY()
	UBitmapPointStorage* Storage;

	/** Memory management and cleanup policies */
	UPROPERTY()
	UBitmapPointMemoryManager* MemoryManager;

	/** Spatial indexing for efficient queries */
	UPROPERTY()
	UBitmapPointSpatialIndex* SpatialIndex;

	/** AR/MR tracking state management */
	UPROPERTY()
	UMRTrackingStateManager* TrackingStateManager;

	/** Real-time update configuration */
	UPROPERTY()
	bool bRealTimeUpdatesEnabled;

	/** Plane detection configuration */
	UPROPERTY(Config)
	bool bAutoPlaneDetectionEnabled;

	UPROPERTY(Config)
	float PlaneDetectionInterval;

	UPROPERTY(Config)
	int32 MinPointsForPlaneDetection;

	/** Internal plane detection tracking */
	float LastPlaneDetectionTime;

private:
	/** Initialize specialized components */
	void InitializeComponents();

	/** Handle events from storage component */
	UFUNCTION()
	void OnStoragePointsChanged(const TArray<FBitmapPoint>& Points);

	/** Handle events from memory manager */
	UFUNCTION()
	void OnMemoryCleanup(int32 PointsRemoved, int32 MemoryFreedKB);

	/** Handle events from spatial index */
	UFUNCTION()
	void OnSpatialIndexUpdated(int32 AddedPoints, int32 RemovedPoints);

	/** Broadcast update to listeners */
	void BroadcastUpdate();

	/** Perform automatic plane detection if enabled */
	void PerformAutoPlaneDetection();
};
