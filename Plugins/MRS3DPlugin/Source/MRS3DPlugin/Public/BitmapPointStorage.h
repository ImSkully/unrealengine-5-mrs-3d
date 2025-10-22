#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BitmapPoint.h"
#include "BitmapPointStorage.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBitmapPointsChanged, const TArray<FBitmapPoint>&, BitmapPoints);

/**
 * Pure storage component for bitmap points with basic CRUD operations
 */
UCLASS(BlueprintType)
class FMRS3DPLUGIN_API UBitmapPointStorage : public UObject
{
	GENERATED_BODY()

public:
	UBitmapPointStorage();

	/**
	 * Add a single bitmap point
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Storage")
	void AddPoint(const FBitmapPoint& Point);

	/**
	 * Add multiple bitmap points
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Storage")
	void AddPoints(const TArray<FBitmapPoint>& Points);

	/**
	 * Remove a point by index
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Storage")
	bool RemovePoint(int32 Index);

	/**
	 * Remove points by predicate
	 */
	int32 RemovePointsWhere(TFunction<bool(const FBitmapPoint&)> Predicate);

	/**
	 * Clear all points
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Storage")
	void Clear();

	/**
	 * Get all points (read-only)
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Storage")
	const TArray<FBitmapPoint>& GetAllPoints() const { return BitmapPoints; }

	/**
	 * Get point count
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Storage")
	int32 GetPointCount() const { return BitmapPoints.Num(); }

	/**
	 * Get point by index
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Storage")
	FBitmapPoint GetPoint(int32 Index) const;

	/**
	 * Check if storage is empty
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Storage")
	bool IsEmpty() const { return BitmapPoints.Num() == 0; }

	/**
	 * Reserve storage capacity
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Storage")
	void Reserve(int32 Capacity);

	/**
	 * Shrink storage to fit
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Storage")
	void Shrink();

	/**
	 * Get memory usage in bytes
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Storage")
	int32 GetMemoryUsageBytes() const;

	/**
	 * Event fired when points are added, removed, or cleared
	 */
	UPROPERTY(BlueprintAssignable, Category = "MRS3D|Storage")
	FOnBitmapPointsChanged OnBitmapPointsChanged;

protected:
	UPROPERTY()
	TArray<FBitmapPoint> BitmapPoints;

	void NotifyPointsChanged();
};