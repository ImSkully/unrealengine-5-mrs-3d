#include "BitmapPointStorage.h"
#include "Engine/Engine.h"

UBitmapPointStorage::UBitmapPointStorage()
{
	BitmapPoints.Reserve(1000); // Default capacity
}

void UBitmapPointStorage::AddPoint(const FBitmapPoint& Point)
{
	BitmapPoints.Add(Point);
	NotifyPointsChanged();
}

void UBitmapPointStorage::AddPoints(const TArray<FBitmapPoint>& Points)
{
	if (Points.Num() == 0)
	{
		return;
	}
	
	BitmapPoints.Append(Points);
	NotifyPointsChanged();
}

bool UBitmapPointStorage::RemovePoint(int32 Index)
{
	if (Index >= 0 && Index < BitmapPoints.Num())
	{
		BitmapPoints.RemoveAt(Index);
		NotifyPointsChanged();
		return true;
	}
	return false;
}

int32 UBitmapPointStorage::RemovePointsWhere(TFunction<bool(const FBitmapPoint&)> Predicate)
{
	const int32 InitialCount = BitmapPoints.Num();
	
	BitmapPoints.RemoveAll([&Predicate](const FBitmapPoint& Point) {
		return Predicate(Point);
	});
	
	const int32 RemovedCount = InitialCount - BitmapPoints.Num();
	if (RemovedCount > 0)
	{
		NotifyPointsChanged();
	}
	
	return RemovedCount;
}

void UBitmapPointStorage::Clear()
{
	if (BitmapPoints.Num() > 0)
	{
		BitmapPoints.Empty();
		NotifyPointsChanged();
	}
}

FBitmapPoint UBitmapPointStorage::GetPoint(int32 Index) const
{
	if (Index >= 0 && Index < BitmapPoints.Num())
	{
		return BitmapPoints[Index];
	}
	return FBitmapPoint(); // Return default point if index is invalid
}

void UBitmapPointStorage::Reserve(int32 Capacity)
{
	BitmapPoints.Reserve(Capacity);
}

void UBitmapPointStorage::Shrink()
{
	BitmapPoints.Shrink();
}

int32 UBitmapPointStorage::GetMemoryUsageBytes() const
{
	const int32 PointSize = sizeof(FBitmapPoint);
	const int32 ArrayOverhead = sizeof(TArray<FBitmapPoint>);
	return (BitmapPoints.Num() * PointSize) + ArrayOverhead;
}

void UBitmapPointStorage::NotifyPointsChanged()
{
	OnBitmapPointsChanged.Broadcast(BitmapPoints);
}