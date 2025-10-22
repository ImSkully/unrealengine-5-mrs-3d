#include "BitmapPointSpatialIndex.h"
#include "Engine/Engine.h"

UBitmapPointSpatialIndex::UBitmapPointSpatialIndex()
	: CellSize(100.0f)
	, WorldBounds(FVector(10000.0f))
	, MaxPointsPerCell(100)
	, TotalPointCount(0)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBitmapPointSpatialIndex::Initialize(float InCellSize, const FVector& InWorldBounds)
{
	CellSize = FMath::Max(1.0f, InCellSize);
	WorldBounds = InWorldBounds;
	
	Clear();
	
	UE_LOG(LogTemp, Log, TEXT("Spatial Index: Initialized with cell size %.1f and bounds %s"), 
		CellSize, *WorldBounds.ToString());
}

void UBitmapPointSpatialIndex::AddPoint(const FBitmapPoint& Point)
{
	const FIntVector GridPos = WorldToGrid(Point.Position);
	FSpatialCell& Cell = GetOrCreateCell(GridPos);
	
	Cell.AddPoint(Point);
	TotalPointCount++;
	
	OnSpatialIndexUpdated.Broadcast(1, 0);
}

void UBitmapPointSpatialIndex::AddPoints(const TArray<FBitmapPoint>& Points)
{
	for (const FBitmapPoint& Point : Points)
	{
		const FIntVector GridPos = WorldToGrid(Point.Position);
		FSpatialCell& Cell = GetOrCreateCell(GridPos);
		Cell.AddPoint(Point);
	}
	
	TotalPointCount += Points.Num();
	OnSpatialIndexUpdated.Broadcast(Points.Num(), 0);
}

bool UBitmapPointSpatialIndex::RemovePoint(const FBitmapPoint& Point)
{
	const FIntVector GridPos = WorldToGrid(Point.Position);
	const FSpatialCell* Cell = GetCell(GridPos);
	
	if (!Cell)
	{
		return false;
	}
	
	FSpatialCell& MutableCell = SpatialGrid[GridPos];
	if (MutableCell.RemovePoint(Point))
	{
		TotalPointCount--;
		
		// Remove empty cell to save memory
		if (MutableCell.IsEmpty())
		{
			SpatialGrid.Remove(GridPos);
		}
		
		OnSpatialIndexUpdated.Broadcast(0, 1);
		return true;
	}
	
	return false;
}

void UBitmapPointSpatialIndex::Clear()
{
	SpatialGrid.Empty();
	TotalPointCount = 0;
	
	OnSpatialIndexUpdated.Broadcast(0, TotalPointCount);
}

TArray<FBitmapPoint> UBitmapPointSpatialIndex::FindPointsInRadius(const FVector& Location, float Radius) const
{
	TArray<FBitmapPoint> Result;
	
	const TArray<FIntVector> CellsToCheck = GetCellsInSphere(Location, Radius);
	
	for (const FIntVector& GridPos : CellsToCheck)
	{
		const FSpatialCell* Cell = GetCell(GridPos);
		if (!Cell)
		{
			continue;
		}
		
		for (const FBitmapPoint& Point : Cell->Points)
		{
			if (IsPointInRadius(Point, Location, Radius))
			{
				Result.Add(Point);
			}
		}
	}
	
	return Result;
}

bool UBitmapPointSpatialIndex::FindNearestPoint(const FVector& Location, FBitmapPoint& OutPoint, float MaxDistance) const
{
	float MinDistanceSquared = MaxDistance * MaxDistance;
	bool bFoundPoint = false;
	
	const TArray<FIntVector> CellsToCheck = GetCellsInSphere(Location, MaxDistance);
	
	for (const FIntVector& GridPos : CellsToCheck)
	{
		const FSpatialCell* Cell = GetCell(GridPos);
		if (!Cell)
		{
			continue;
		}
		
		for (const FBitmapPoint& Point : Cell->Points)
		{
			const float DistanceSquared = GetSquaredDistance(Point, Location);
			if (DistanceSquared < MinDistanceSquared)
			{
				MinDistanceSquared = DistanceSquared;
				OutPoint = Point;
				bFoundPoint = true;
			}
		}
	}
	
	return bFoundPoint;
}

TArray<FBitmapPoint> UBitmapPointSpatialIndex::FindKNearestPoints(const FVector& Location, int32 K, float MaxDistance) const
{
	TArray<FBitmapPoint> Result;
	
	if (K <= 0)
	{
		return Result;
	}
	
	// Use a priority queue to maintain K nearest points
	struct FPointDistance
	{
		FBitmapPoint Point;
		float DistanceSquared;
		
		bool operator<(const FPointDistance& Other) const
		{
			return DistanceSquared > Other.DistanceSquared; // Max heap (furthest first)
		}
	};
	
	TArray<FPointDistance> NearestPoints;
	const float MaxDistanceSquared = MaxDistance * MaxDistance;
	
	const TArray<FIntVector> CellsToCheck = GetCellsInSphere(Location, MaxDistance);
	
	for (const FIntVector& GridPos : CellsToCheck)
	{
		const FSpatialCell* Cell = GetCell(GridPos);
		if (!Cell)
		{
			continue;
		}
		
		for (const FBitmapPoint& Point : Cell->Points)
		{
			const float DistanceSquared = GetSquaredDistance(Point, Location);
			
			if (DistanceSquared <= MaxDistanceSquared)
			{
				if (NearestPoints.Num() < K)
				{
					NearestPoints.Add({Point, DistanceSquared});
					NearestPoints.HeapifyUp(NearestPoints.Num() - 1);
				}
				else if (DistanceSquared < NearestPoints[0].DistanceSquared)
				{
					NearestPoints[0] = {Point, DistanceSquared};
					NearestPoints.Heapify();
				}
			}
		}
	}
	
	// Extract points from heap
	Result.Reserve(NearestPoints.Num());
	for (const FPointDistance& PointDist : NearestPoints)
	{
		Result.Add(PointDist.Point);
	}
	
	// Sort by distance (closest first)
	Result.Sort([&Location](const FBitmapPoint& A, const FBitmapPoint& B) {
		const float DistA = FVector::DistSquared(A.Position, Location);
		const float DistB = FVector::DistSquared(B.Position, Location);
		return DistA < DistB;
	});
	
	return Result;
}

TArray<FBitmapPoint> UBitmapPointSpatialIndex::FindPointsInBox(const FVector& MinBounds, const FVector& MaxBounds) const
{
	TArray<FBitmapPoint> Result;
	
	const TArray<FIntVector> CellsToCheck = GetCellsInBox(MinBounds, MaxBounds);
	
	for (const FIntVector& GridPos : CellsToCheck)
	{
		const FSpatialCell* Cell = GetCell(GridPos);
		if (!Cell)
		{
			continue;
		}
		
		for (const FBitmapPoint& Point : Cell->Points)
		{
			if (Point.Position.X >= MinBounds.X && Point.Position.X <= MaxBounds.X &&
				Point.Position.Y >= MinBounds.Y && Point.Position.Y <= MaxBounds.Y &&
				Point.Position.Z >= MinBounds.Z && Point.Position.Z <= MaxBounds.Z)
			{
				Result.Add(Point);
			}
		}
	}
	
	return Result;
}

TArray<FBitmapPoint> UBitmapPointSpatialIndex::FindPointsAlongRay(const FVector& Origin, const FVector& Direction, float Tolerance, float MaxDistance) const
{
	TArray<FBitmapPoint> Result;
	
	const FVector NormalizedDirection = Direction.GetSafeNormal();
	const FVector EndPoint = Origin + NormalizedDirection * MaxDistance;
	
	// Create bounding box around the ray
	const FVector MinBounds = FVector(
		FMath::Min(Origin.X, EndPoint.X) - Tolerance,
		FMath::Min(Origin.Y, EndPoint.Y) - Tolerance,
		FMath::Min(Origin.Z, EndPoint.Z) - Tolerance
	);
	
	const FVector MaxBounds = FVector(
		FMath::Max(Origin.X, EndPoint.X) + Tolerance,
		FMath::Max(Origin.Y, EndPoint.Y) + Tolerance,
		FMath::Max(Origin.Z, EndPoint.Z) + Tolerance
	);
	
	const TArray<FIntVector> CellsToCheck = GetCellsInBox(MinBounds, MaxBounds);
	
	for (const FIntVector& GridPos : CellsToCheck)
	{
		const FSpatialCell* Cell = GetCell(GridPos);
		if (!Cell)
		{
			continue;
		}
		
		for (const FBitmapPoint& Point : Cell->Points)
		{
			// Calculate distance from point to ray
			const FVector PointToOrigin = Point.Position - Origin;
			const float ProjectedDistance = FVector::DotProduct(PointToOrigin, NormalizedDirection);
			
			if (ProjectedDistance >= 0.0f && ProjectedDistance <= MaxDistance)
			{
				const FVector ClosestPointOnRay = Origin + NormalizedDirection * ProjectedDistance;
				const float DistanceToRay = FVector::Dist(Point.Position, ClosestPointOnRay);
				
				if (DistanceToRay <= Tolerance)
				{
					Result.Add(Point);
				}
			}
		}
	}
	
	return Result;
}

int32 UBitmapPointSpatialIndex::GetMemoryUsageBytes() const
{
	int32 MemoryUsage = sizeof(*this);
	
	// Add memory for spatial grid
	MemoryUsage += SpatialGrid.GetAllocatedSize();
	
	// Add memory for points in cells
	for (const auto& GridPair : SpatialGrid)
	{
		MemoryUsage += GridPair.Value.Points.GetAllocatedSize();
	}
	
	return MemoryUsage;
}

void UBitmapPointSpatialIndex::GetSpatialStats(int32& ActiveCells, int32& MaxPointsPerCell, float& AveragePointsPerCell) const
{
	ActiveCells = SpatialGrid.Num();
	MaxPointsPerCell = 0;
	int32 TotalPoints = 0;
	
	for (const auto& GridPair : SpatialGrid)
	{
		const int32 PointsInCell = GridPair.Value.Points.Num();
		MaxPointsPerCell = FMath::Max(MaxPointsPerCell, PointsInCell);
		TotalPoints += PointsInCell;
	}
	
	AveragePointsPerCell = ActiveCells > 0 ? static_cast<float>(TotalPoints) / ActiveCells : 0.0f;
}

void UBitmapPointSpatialIndex::Rebuild()
{
	TArray<FBitmapPoint> AllPoints;
	
	// Collect all points
	for (const auto& GridPair : SpatialGrid)
	{
		AllPoints.Append(GridPair.Value.Points);
	}
	
	// Clear and re-add
	Clear();
	AddPoints(AllPoints);
	
	UE_LOG(LogTemp, Log, TEXT("Spatial Index: Rebuilt with %d points"), AllPoints.Num());
}

FIntVector UBitmapPointSpatialIndex::WorldToGrid(const FVector& WorldPos) const
{
	return FIntVector(
		FMath::FloorToInt(WorldPos.X / CellSize),
		FMath::FloorToInt(WorldPos.Y / CellSize),
		FMath::FloorToInt(WorldPos.Z / CellSize)
	);
}

FVector UBitmapPointSpatialIndex::GridToWorld(const FIntVector& GridPos) const
{
	return FVector(
		GridPos.X * CellSize,
		GridPos.Y * CellSize,
		GridPos.Z * CellSize
	);
}

TArray<FIntVector> UBitmapPointSpatialIndex::GetCellsInSphere(const FVector& Center, float Radius) const
{
	TArray<FIntVector> Cells;
	
	const FIntVector CenterGrid = WorldToGrid(Center);
	const int32 CellRadius = FMath::CeilToInt(Radius / CellSize);
	
	for (int32 X = -CellRadius; X <= CellRadius; X++)
	{
		for (int32 Y = -CellRadius; Y <= CellRadius; Y++)
		{
			for (int32 Z = -CellRadius; Z <= CellRadius; Z++)
			{
				const FIntVector GridPos = CenterGrid + FIntVector(X, Y, Z);
				const FVector CellCenter = GridToWorld(GridPos) + FVector(CellSize * 0.5f);
				
				// Check if cell intersects with sphere
				const float DistanceToCell = FVector::Dist(Center, CellCenter);
				const float CellDiagonal = CellSize * FMath::Sqrt(3.0f) * 0.5f;
				
				if (DistanceToCell <= Radius + CellDiagonal)
				{
					Cells.Add(GridPos);
				}
			}
		}
	}
	
	return Cells;
}

TArray<FIntVector> UBitmapPointSpatialIndex::GetCellsInBox(const FVector& MinBounds, const FVector& MaxBounds) const
{
	TArray<FIntVector> Cells;
	
	const FIntVector MinGrid = WorldToGrid(MinBounds);
	const FIntVector MaxGrid = WorldToGrid(MaxBounds);
	
	for (int32 X = MinGrid.X; X <= MaxGrid.X; X++)
	{
		for (int32 Y = MinGrid.Y; Y <= MaxGrid.Y; Y++)
		{
			for (int32 Z = MinGrid.Z; Z <= MaxGrid.Z; Z++)
			{
				Cells.Add(FIntVector(X, Y, Z));
			}
		}
	}
	
	return Cells;
}

UBitmapPointSpatialIndex::FSpatialCell& UBitmapPointSpatialIndex::GetOrCreateCell(const FIntVector& GridPos)
{
	return SpatialGrid.FindOrAdd(GridPos);
}

const UBitmapPointSpatialIndex::FSpatialCell* UBitmapPointSpatialIndex::GetCell(const FIntVector& GridPos) const
{
	return SpatialGrid.Find(GridPos);
}

bool UBitmapPointSpatialIndex::IsPointInRadius(const FBitmapPoint& Point, const FVector& Center, float Radius) const
{
	return FVector::DistSquared(Point.Position, Center) <= (Radius * Radius);
}

float UBitmapPointSpatialIndex::GetSquaredDistance(const FBitmapPoint& Point, const FVector& Location) const
{
	return FVector::DistSquared(Point.Position, Location);
}