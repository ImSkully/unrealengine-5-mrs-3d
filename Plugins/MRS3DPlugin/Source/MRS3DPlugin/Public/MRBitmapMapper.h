#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BitmapPoint.h"
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
	 * Event fired when bitmap points are updated
	 */
	UPROPERTY(BlueprintAssignable, Category = "MRS3D|MR")
	FOnBitmapPointsUpdated OnBitmapPointsUpdated;

protected:
	UPROPERTY()
	TArray<FBitmapPoint> BitmapPoints;

	UPROPERTY()
	bool bRealTimeUpdatesEnabled;

	void BroadcastUpdate();
};
