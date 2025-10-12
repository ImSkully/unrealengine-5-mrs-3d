#include "MRBitmapMapper.h"
#include "Engine/Engine.h"

UMRBitmapMapper::UMRBitmapMapper()
	: bRealTimeUpdatesEnabled(true)
{
}

void UMRBitmapMapper::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("MRBitmapMapper Subsystem Initialized"));
	BitmapPoints.Empty();
}

void UMRBitmapMapper::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("MRBitmapMapper Subsystem Deinitialized"));
	BitmapPoints.Empty();
	Super::Deinitialize();
}

void UMRBitmapMapper::AddBitmapPoint(const FBitmapPoint& Point)
{
	BitmapPoints.Add(Point);
	
	if (bRealTimeUpdatesEnabled)
	{
		BroadcastUpdate();
	}
}

void UMRBitmapMapper::AddBitmapPoints(const TArray<FBitmapPoint>& Points)
{
	BitmapPoints.Append(Points);
	
	if (bRealTimeUpdatesEnabled)
	{
		BroadcastUpdate();
	}
}

void UMRBitmapMapper::ClearBitmapPoints()
{
	BitmapPoints.Empty();
	BroadcastUpdate();
}

TArray<FBitmapPoint> UMRBitmapMapper::GetBitmapPointsInRadius(const FVector& Center, float Radius) const
{
	TArray<FBitmapPoint> PointsInRadius;
	const float RadiusSquared = Radius * Radius;

	for (const FBitmapPoint& Point : BitmapPoints)
	{
		if (FVector::DistSquared(Point.Position, Center) <= RadiusSquared)
		{
			PointsInRadius.Add(Point);
		}
	}

	return PointsInRadius;
}

void UMRBitmapMapper::SetRealTimeUpdates(bool bEnabled)
{
	bRealTimeUpdatesEnabled = bEnabled;
}

void UMRBitmapMapper::BroadcastUpdate()
{
	OnBitmapPointsUpdated.Broadcast(BitmapPoints);
}
