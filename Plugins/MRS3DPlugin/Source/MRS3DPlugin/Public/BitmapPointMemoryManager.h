#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BitmapPointStorage.h"
#include "BitmapPointMemoryManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMemoryCleanup, int32, PointsRemoved, int32, MemoryFreedKB);

/**
 * Memory management component for bitmap points with automatic cleanup
 */
UCLASS(BlueprintType)
class FMRS3DPLUGIN_API UBitmapPointMemoryManager : public UObject
{
	GENERATED_BODY()

public:
	UBitmapPointMemoryManager();

	/**
	 * Initialize with storage reference
	 */
	void Initialize(UBitmapPointStorage* InStorage);

	/**
	 * Set maximum number of points allowed
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Memory")
	void SetMaxPoints(int32 MaxPoints);

	/**
	 * Set maximum age for points in seconds
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Memory")
	void SetMaxPointAge(float MaxAgeSeconds);

	/**
	 * Enable/disable automatic cleanup
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Memory")
	void SetAutoCleanupEnabled(bool bEnabled);

	/**
	 * Set cleanup interval in seconds
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Memory")
	void SetCleanupInterval(float IntervalSeconds);

	/**
	 * Manually trigger cleanup
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Memory")
	int32 PerformCleanup();

	/**
	 * Remove old points based on age
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Memory")
	int32 RemoveOldPoints();

	/**
	 * Remove excess points to stay within limit
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Memory")
	int32 RemoveExcessPoints();

	/**
	 * Check if cleanup is needed
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Memory")
	bool ShouldPerformCleanup() const;

	/**
	 * Get current memory usage in KB
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Memory")
	int32 GetMemoryUsageKB() const;

	/**
	 * Get cleanup statistics
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Memory")
	void GetCleanupStats(int32& TotalCleanupsPerformed, int32& TotalPointsRemoved, int32& TotalMemoryFreedKB) const;

	/**
	 * Update memory manager (should be called regularly)
	 */
	void Tick(float DeltaTime);

	/**
	 * Event fired when cleanup is performed
	 */
	UPROPERTY(BlueprintAssignable, Category = "MRS3D|Memory")
	FOnMemoryCleanup OnMemoryCleanup;

protected:
	UPROPERTY()
	UBitmapPointStorage* Storage;

	// Configuration
	UPROPERTY(Config)
	int32 MaxBitmapPoints;

	UPROPERTY(Config)
	float MaxPointAgeSeconds;

	UPROPERTY(Config)
	bool bAutoCleanupEnabled;

	UPROPERTY(Config)
	float CleanupIntervalSeconds;

	// Internal state
	float LastCleanupTime;
	int32 CleanupCount;
	int32 TotalPointsRemoved;
	int32 TotalMemoryFreed;

	/**
	 * Internal cleanup logic
	 */
	int32 PerformCleanupInternal();
};