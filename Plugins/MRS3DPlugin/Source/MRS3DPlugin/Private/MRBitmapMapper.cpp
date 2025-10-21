#include "MRBitmapMapper.h"
#include "PlaneDetectionSubsystem.h"
#include "Engine/Engine.h"

UMRBitmapMapper::UMRBitmapMapper()
	: bRealTimeUpdatesEnabled(true)
	, MaxBitmapPoints(50000)  // Default limit of 50k points
	, MaxPointAgeSeconds(300.0f)  // 5 minutes default
	, bAutoCleanupEnabled(true)
	, CleanupIntervalSeconds(30.0f)  // Cleanup every 30 seconds
	, LastCleanupTime(0.0f)
	, bAutoPlaneDetectionEnabled(false)
	, PlaneDetectionInterval(10.0f)  // Detect planes every 10 seconds
	, MinPointsForPlaneDetection(100)
	, LastPlaneDetectionTime(0.0f)
	, CurrentTrackingState(ETrackingState::NotTracking)
{

void UMRBitmapMapper::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("MRBitmapMapper Subsystem Initialized"));
	BitmapPoints.Empty();
	BitmapPoints.Reserve(1000); // Reserve space for better performance
	LastCleanupTime = FPlatformTime::Seconds();
	LastPlaneDetectionTime = FPlatformTime::Seconds();
	CurrentTrackingState = ETrackingState::NotTracking;
}

void UMRBitmapMapper::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("MRBitmapMapper Subsystem Deinitialized"));
	BitmapPoints.Empty();
	Super::Deinitialize();
}

void UMRBitmapMapper::AddBitmapPoint(const FBitmapPoint& Point)
{
	// Perform auto cleanup if enabled
	if (bAutoCleanupEnabled)
	{
		PerformAutoCleanup();
	}

	// Perform auto plane detection if enabled
	if (bAutoPlaneDetectionEnabled)
	{
		PerformAutoPlaneDetection();
	}

	BitmapPoints.Add(Point);
	
	// Check if we exceed max points limit
	if (MaxBitmapPoints > 0 && BitmapPoints.Num() > MaxBitmapPoints)
	{
		RemoveExcessPoints();
	}
	
	if (bRealTimeUpdatesEnabled)
	{
		BroadcastUpdate();
	}
}

void UMRBitmapMapper::AddBitmapPoints(const TArray<FBitmapPoint>& Points)
{
	// Perform auto cleanup if enabled
	if (bAutoCleanupEnabled)
	{
		PerformAutoCleanup();
	}

	BitmapPoints.Append(Points);
	
	// Check if we exceed max points limit
	if (MaxBitmapPoints > 0 && BitmapPoints.Num() > MaxBitmapPoints)
	{
		RemoveExcessPoints();
	}
	
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

void UMRBitmapMapper::SetMaxBitmapPoints(int32 MaxPoints)
{
	MaxBitmapPoints = FMath::Max(0, MaxPoints);
	
	// Immediately remove excess points if needed
	if (MaxBitmapPoints > 0 && BitmapPoints.Num() > MaxBitmapPoints)
	{
		RemoveExcessPoints();
	}
}

void UMRBitmapMapper::SetMaxPointAge(float MaxAgeSeconds)
{
	MaxPointAgeSeconds = FMath::Max(0.0f, MaxAgeSeconds);
}

int32 UMRBitmapMapper::RemoveOldPoints()
{
	const float CurrentTime = FPlatformTime::Seconds();
	const int32 InitialCount = BitmapPoints.Num();
	
	RemovePointsByAge(CurrentTime);
	
	const int32 RemovedCount = InitialCount - BitmapPoints.Num();
	if (RemovedCount > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Removed %d old bitmap points"), RemovedCount);
		BroadcastUpdate();
	}
	
	return RemovedCount;
}

int32 UMRBitmapMapper::GetMemoryUsageKB() const
{
	// Calculate approximate memory usage
	const int32 PointSize = sizeof(FBitmapPoint);
	const int32 ArrayOverhead = sizeof(TArray<FBitmapPoint>);
	const int32 TotalBytes = (BitmapPoints.Num() * PointSize) + ArrayOverhead;
	return TotalBytes / 1024;
}

void UMRBitmapMapper::ForceCleanup()
{
	const float CurrentTime = FPlatformTime::Seconds();
	const int32 InitialCount = BitmapPoints.Num();
	
	// Remove old points
	RemovePointsByAge(CurrentTime);
	
	// Remove excess points
	if (MaxBitmapPoints > 0 && BitmapPoints.Num() > MaxBitmapPoints)
	{
		RemoveExcessPoints();
	}
	
	// Shrink array to fit
	BitmapPoints.Shrink();
	
	const int32 RemovedCount = InitialCount - BitmapPoints.Num();
	UE_LOG(LogTemp, Log, TEXT("Force cleanup removed %d points, memory usage: %d KB"), 
		RemovedCount, GetMemoryUsageKB());
	
	LastCleanupTime = CurrentTime;
	
	if (RemovedCount > 0)
	{
		BroadcastUpdate();
	}
}

void UMRBitmapMapper::PerformAutoCleanup()
{
	const float CurrentTime = FPlatformTime::Seconds();
	
	// Check if it's time for cleanup
	if (CurrentTime - LastCleanupTime >= CleanupIntervalSeconds)
	{
		RemovePointsByAge(CurrentTime);
		LastCleanupTime = CurrentTime;
	}
}

void UMRBitmapMapper::RemoveExcessPoints()
{
	if (MaxBitmapPoints <= 0 || BitmapPoints.Num() <= MaxBitmapPoints)
	{
		return;
	}
	
	// Remove oldest points first (FIFO)
	const int32 PointsToRemove = BitmapPoints.Num() - MaxBitmapPoints;
	
	// Sort by timestamp to remove oldest first
	BitmapPoints.Sort([](const FBitmapPoint& A, const FBitmapPoint& B) {
		return A.Timestamp < B.Timestamp;
	});
	
	// Remove the oldest points
	BitmapPoints.RemoveAt(0, PointsToRemove);
	
	UE_LOG(LogTemp, Warning, TEXT("Removed %d excess bitmap points to stay within limit of %d"), 
		PointsToRemove, MaxBitmapPoints);
}

void UMRBitmapMapper::RemovePointsByAge(float CurrentTime)
{
	if (MaxPointAgeSeconds <= 0.0f)
	{
		return;
	}
	
	const float OldestAllowedTime = CurrentTime - MaxPointAgeSeconds;
	const int32 InitialCount = BitmapPoints.Num();
	
	// Remove points that are too old
	BitmapPoints.RemoveAll([OldestAllowedTime](const FBitmapPoint& Point) {
		return Point.Timestamp < OldestAllowedTime;
	});
	
	const int32 RemovedCount = InitialCount - BitmapPoints.Num();
	if (RemovedCount > 0)
	{
		UE_LOG(LogTemp, Verbose, TEXT("Auto-cleanup removed %d aged bitmap points"), RemovedCount);
	}
}

void UMRBitmapMapper::SetAutoPlaneDetectionEnabled(bool bEnabled)
{
	bAutoPlaneDetectionEnabled = bEnabled;
	UE_LOG(LogTemp, Log, TEXT("Auto plane detection %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

TArray<FDetectedPlane> UMRBitmapMapper::DetectPlanesFromCurrentPoints(float PlaneThickness)
{
	if (UPlaneDetectionSubsystem* PlaneSubsystem = GetGameInstance()->GetSubsystem<UPlaneDetectionSubsystem>())
	{
		TArray<FDetectedPlane> DetectedPlanes = PlaneSubsystem->DetectPlanesFromPoints(BitmapPoints, PlaneThickness);
		
		// Add detected planes to the plane detection subsystem
		for (const FDetectedPlane& Plane : DetectedPlanes)
		{
			PlaneSubsystem->AddDetectedPlane(Plane);
		}
		
		UE_LOG(LogTemp, Log, TEXT("Detected %d planes from %d bitmap points"), DetectedPlanes.Num(), BitmapPoints.Num());
		return DetectedPlanes;
	}
	
	return TArray<FDetectedPlane>();
}

void UMRBitmapMapper::UpdateARTrackingState(ETrackingState NewState, float Quality, const FString& LossReason)
{
	ETrackingState PreviousState = CurrentTrackingState;
	CurrentTrackingState = NewState;
	
	// Update plane detection subsystem with tracking state
	if (UPlaneDetectionSubsystem* PlaneSubsystem = GetGameInstance()->GetSubsystem<UPlaneDetectionSubsystem>())
	{
		PlaneSubsystem->UpdateTrackingState(NewState, Quality, LossReason);
	}
	
	UE_LOG(LogTemp, Log, TEXT("AR Tracking state updated from %d to %d (Quality: %.2f)"), 
		(int32)PreviousState, (int32)NewState, Quality);
	
	// Handle tracking loss scenarios
	if (NewState == ETrackingState::TrackingLost || NewState == ETrackingState::NotTracking)
	{
		// Stop automatic plane detection during tracking loss
		if (bAutoPlaneDetectionEnabled)
		{
			UE_LOG(LogTemp, Warning, TEXT("Pausing auto plane detection due to tracking loss"));
		}
	}
	else if (NewState == ETrackingState::FullTracking && PreviousState != ETrackingState::FullTracking)
	{
		// Resume plane detection when tracking is restored
		if (bAutoPlaneDetectionEnabled)
		{
			LastPlaneDetectionTime = FPlatformTime::Seconds(); // Reset timer
			UE_LOG(LogTemp, Log, TEXT("Resuming auto plane detection with full tracking"));
		}
	}
}

ETrackingState UMRBitmapMapper::GetCurrentTrackingState() const
{
	return CurrentTrackingState;
}

void UMRBitmapMapper::PerformAutoPlaneDetection()
{
	const float CurrentTime = FPlatformTime::Seconds();
	
	// Check if it's time for plane detection and we have enough points
	if (CurrentTime - LastPlaneDetectionTime >= PlaneDetectionInterval &&
		BitmapPoints.Num() >= MinPointsForPlaneDetection &&
		CurrentTrackingState == ETrackingState::FullTracking)
	{
		DetectPlanesFromCurrentPoints();
		LastPlaneDetectionTime = CurrentTime;
	}
}
