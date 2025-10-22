#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BitmapPoint.h"
#include "Components/ActorComponent.h"
#include "BitmapPointSpatialIndex.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpatialIndexUpdated, int32, AddedPoints, int32, RemovedPoints);

/**
 * Spatial index for efficient bitmap point queries
 * Provides fast radius-based searches and nearest neighbor queries
 */
UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class MRS3DPLUGIN_API UBitmapPointSpatialIndex : public UActorComponent
{
	GENERATED_BODY()

public:
	UBitmapPointSpatialIndex();

	/** Initialize the spatial index with grid configuration */
	UFUNCTION(BlueprintCallable, Category = "Spatial Index")
	void Initialize(float InCellSize = 100.0f, const FVector& InWorldBounds = FVector(10000.0f));

	/** Add a single point to the spatial index */
	UFUNCTION(BlueprintCallable, Category = "Spatial Index")
	void AddPoint(const FBitmapPoint& Point);

	/** Add multiple points to the spatial index */
	UFUNCTION(BlueprintCallable, Category = "Spatial Index")
	void AddPoints(const TArray<FBitmapPoint>& Points);

	/** Remove a point from the spatial index */
	UFUNCTION(BlueprintCallable, Category = "Spatial Index")
	bool RemovePoint(const FBitmapPoint& Point);

	/** Remove points matching a predicate */
	template<typename Predicate>
	int32 RemovePointsWhere(Predicate Pred);

	/** Clear all points from the spatial index */
	UFUNCTION(BlueprintCallable, Category = "Spatial Index")
	void Clear();

	/** Find all points within radius of a location */
	UFUNCTION(BlueprintCallable, Category = "Spatial Index")
	TArray<FBitmapPoint> FindPointsInRadius(const FVector& Location, float Radius) const;

	/** Find the nearest point to a location */
	UFUNCTION(BlueprintCallable, Category = "Spatial Index")
	bool FindNearestPoint(const FVector& Location, FBitmapPoint& OutPoint, float MaxDistance = 1000.0f) const;

	/** Find the K nearest points to a location */
	UFUNCTION(BlueprintCallable, Category = "Spatial Index")
	TArray<FBitmapPoint> FindKNearestPoints(const FVector& Location, int32 K, float MaxDistance = 1000.0f) const;

	/** Get all points in a bounding box */
	UFUNCTION(BlueprintCallable, Category = "Spatial Index")
	TArray<FBitmapPoint> FindPointsInBox(const FVector& MinBounds, const FVector& MaxBounds) const;

	/** Get points along a ray with tolerance */
	UFUNCTION(BlueprintCallable, Category = "Spatial Index")
	TArray<FBitmapPoint> FindPointsAlongRay(const FVector& Origin, const FVector& Direction, float Tolerance = 10.0f, float MaxDistance = 1000.0f) const;

	/** Get the total number of indexed points */
	UFUNCTION(BlueprintCallable, Category = "Spatial Index")
	int32 GetPointCount() const { return TotalPointCount; }

	/** Get memory usage statistics */
	UFUNCTION(BlueprintCallable, Category = "Spatial Index")
	int32 GetMemoryUsageBytes() const;

	/** Get spatial distribution statistics */
	UFUNCTION(BlueprintCallable, Category = "Spatial Index")
	void GetSpatialStats(int32& ActiveCells, int32& MaxPointsPerCell, float& AveragePointsPerCell) const;

	/** Rebuild the spatial index (useful after major changes) */
	UFUNCTION(BlueprintCallable, Category = "Spatial Index")
	void Rebuild();

	/** Event fired when spatial index is updated */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSpatialIndexUpdated OnSpatialIndexUpdated;

protected:
	/** Size of each spatial grid cell */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float CellSize;

	/** World bounds for the spatial index */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	FVector WorldBounds;

	/** Maximum points per cell before subdivision (future optimization) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	int32 MaxPointsPerCell;

private:
	/** Spatial grid structure */
	struct FSpatialCell
	{
		TArray<FBitmapPoint> Points;
		
		FSpatialCell() = default;
		
		void AddPoint(const FBitmapPoint& Point)
		{
			Points.Add(Point);
		}
		
		bool RemovePoint(const FBitmapPoint& Point)
		{
			return Points.RemoveSingle(Point) > 0;
		}
		
		void Clear()
		{
			Points.Empty();
		}
		
		bool IsEmpty() const
		{
			return Points.Num() == 0;
		}
	};

	/** 3D grid of spatial cells */
	TMap<FIntVector, FSpatialCell> SpatialGrid;

	/** Total number of points in the index */
	int32 TotalPointCount;

	/** Convert world position to grid coordinates */
	FIntVector WorldToGrid(const FVector& WorldPos) const;

	/** Convert grid coordinates to world position */
	FVector GridToWorld(const FIntVector& GridPos) const;

	/** Get all grid cells that intersect with a sphere */
	TArray<FIntVector> GetCellsInSphere(const FVector& Center, float Radius) const;

	/** Get all grid cells that intersect with a bounding box */
	TArray<FIntVector> GetCellsInBox(const FVector& MinBounds, const FVector& MaxBounds) const;

	/** Get cell for a world position, creating if necessary */
	FSpatialCell& GetOrCreateCell(const FIntVector& GridPos);

	/** Get cell for a world position (const version) */
	const FSpatialCell* GetCell(const FIntVector& GridPos) const;

	/** Helper function to check if a point is within distance */
	bool IsPointInRadius(const FBitmapPoint& Point, const FVector& Center, float Radius) const;

	/** Helper function to get squared distance between point and location */
	float GetSquaredDistance(const FBitmapPoint& Point, const FVector& Location) const;
};

template<typename Predicate>
int32 UBitmapPointSpatialIndex::RemovePointsWhere(Predicate Pred)
{
	int32 RemovedCount = 0;
	
	for (auto& GridPair : SpatialGrid)
	{
		FSpatialCell& Cell = GridPair.Value;
		const int32 InitialCount = Cell.Points.Num();
		
		Cell.Points.RemoveAll(Pred);
		
		const int32 CellRemovedCount = InitialCount - Cell.Points.Num();
		RemovedCount += CellRemovedCount;
	}
	
	// Remove empty cells to save memory
	for (auto It = SpatialGrid.CreateIterator(); It; ++It)
	{
		if (It.Value().IsEmpty())
		{
			It.RemoveCurrent();
		}
	}
	
	TotalPointCount -= RemovedCount;
	
	if (RemovedCount > 0)
	{
		OnSpatialIndexUpdated.Broadcast(0, RemovedCount);
	}
	
	return RemovedCount;
}