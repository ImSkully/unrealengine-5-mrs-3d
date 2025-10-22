#include "MRBitmapMapper.h"
#include "PlaneDetectionSubsystem.h"
#include "Engine/Engine.h"

UMRBitmapMapper::UMRBitmapMapper()
	: Storage(nullptr)
	, MemoryManager(nullptr)
	, SpatialIndex(nullptr)
	, TrackingStateManager(nullptr)
	, bRealTimeUpdatesEnabled(true)
	, bAutoPlaneDetectionEnabled(false)
	, PlaneDetectionInterval(10.0f)
	, MinPointsForPlaneDetection(100)
	, LastPlaneDetectionTime(0.0f)
{
}

void UMRBitmapMapper::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	InitializeComponents();
	
	UE_LOG(LogTemp, Log, TEXT("MRBitmapMapper: Initialized with specialized components"));
}

void UMRBitmapMapper::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("MRBitmapMapper: Deinitialized"));
	
	// Cleanup will be handled by component destructors
	Storage = nullptr;
	MemoryManager = nullptr;
	SpatialIndex = nullptr;
	TrackingStateManager = nullptr;
	
	Super::Deinitialize();
}

void UMRBitmapMapper::AddBitmapPoint(const FBitmapPoint& Point)
{
	if (!Storage || !bRealTimeUpdatesEnabled)
	{
		return;
	}

	// Add to storage (which will trigger events)
	Storage->AddPoint(Point);
	
	// Add to spatial index for fast queries
	if (SpatialIndex)
	{
		SpatialIndex->AddPoint(Point);
	}
	
	// Perform auto plane detection if enabled
	if (bAutoPlaneDetectionEnabled)
	{
		PerformAutoPlaneDetection();
	}
}

void UMRBitmapMapper::AddBitmapPoints(const TArray<FBitmapPoint>& Points)
{
	if (!Storage || !bRealTimeUpdatesEnabled || Points.Num() == 0)
	{
		return;
	}

	// Add to storage (which will trigger events)
	Storage->AddPoints(Points);
	
	// Add to spatial index for fast queries
	if (SpatialIndex)
	{
		SpatialIndex->AddPoints(Points);
	}
	
	// Perform auto plane detection if enabled
	if (bAutoPlaneDetectionEnabled)
	{
		PerformAutoPlaneDetection();
	}
}

void UMRBitmapMapper::ClearBitmapPoints()
{
	if (Storage)
	{
		Storage->Clear();
	}
	
	if (SpatialIndex)
	{
		SpatialIndex->Clear();
	}
}

const TArray<FBitmapPoint>& UMRBitmapMapper::GetBitmapPoints() const
{
	static TArray<FBitmapPoint> EmptyArray;
	return Storage ? Storage->GetAllPoints() : EmptyArray;
}

TArray<FBitmapPoint> UMRBitmapMapper::GetBitmapPointsInRadius(const FVector& Center, float Radius) const
{
	if (!SpatialIndex)
	{
		return TArray<FBitmapPoint>();
	}
	
	return SpatialIndex->FindPointsInRadius(Center, Radius);
}

bool UMRBitmapMapper::FindNearestPoint(const FVector& Location, FBitmapPoint& OutPoint, float MaxDistance) const
{
	if (!SpatialIndex)
	{
		return false;
	}
	
	return SpatialIndex->FindNearestPoint(Location, OutPoint, MaxDistance);
}

TArray<FBitmapPoint> UMRBitmapMapper::FindKNearestPoints(const FVector& Location, int32 K, float MaxDistance) const
{
	if (!SpatialIndex)
	{
		return TArray<FBitmapPoint>();
	}
	
	return SpatialIndex->FindKNearestPoints(Location, K, MaxDistance);
}

void UMRBitmapMapper::SetRealTimeUpdates(bool bEnabled)
{
	bRealTimeUpdatesEnabled = bEnabled;
	UE_LOG(LogTemp, Log, TEXT("MRBitmapMapper: Real-time updates %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void UMRBitmapMapper::SetMaxBitmapPoints(int32 MaxPoints)
{
	if (MemoryManager)
	{
		MemoryManager->SetMaxPoints(MaxPoints);
	}
}

void UMRBitmapMapper::SetMaxPointAge(float MaxAgeSeconds)
{
	if (MemoryManager)
	{
		MemoryManager->SetMaxPointAge(MaxAgeSeconds);
	}
}

int32 UMRBitmapMapper::RemoveOldPoints()
{
	if (!MemoryManager)
	{
		return 0;
	}
	
	return MemoryManager->RemoveOldPoints();
}

int32 UMRBitmapMapper::GetMemoryUsageKB() const
{
	int32 TotalMemory = 0;
	
	if (Storage)
	{
		TotalMemory += Storage->GetMemoryUsageBytes() / 1024;
	}
	
	if (SpatialIndex)
	{
		TotalMemory += SpatialIndex->GetMemoryUsageBytes() / 1024;
	}
	
	return TotalMemory;
}

void UMRBitmapMapper::ForceCleanup()
{
	if (MemoryManager)
	{
		MemoryManager->PerformCleanup();
	}
}

void UMRBitmapMapper::SetAutoPlaneDetectionEnabled(bool bEnabled)
{
	bAutoPlaneDetectionEnabled = bEnabled;
	UE_LOG(LogTemp, Log, TEXT("MRBitmapMapper: Auto plane detection %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

TArray<FDetectedPlane> UMRBitmapMapper::DetectPlanesFromCurrentPoints(float PlaneThickness)
{
	UPlaneDetectionSubsystem* PlaneDetection = GetGameInstance()->GetSubsystem<UPlaneDetectionSubsystem>();
	if (!PlaneDetection || !Storage)
	{
		return TArray<FDetectedPlane>();
	}
	
	const TArray<FBitmapPoint>& CurrentPoints = Storage->GetAllPoints();
	return PlaneDetection->DetectPlanes(CurrentPoints, PlaneThickness);
}

void UMRBitmapMapper::UpdateARTrackingState(ETrackingState NewState, const FTransform& CameraPose, float Quality)
{
	if (TrackingStateManager)
	{
		TrackingStateManager->UpdateTrackingState(NewState, CameraPose, Quality);
	}
}

ETrackingState UMRBitmapMapper::GetCurrentTrackingState() const
{
	if (!TrackingStateManager)
	{
		return ETrackingState::NotTracking;
	}
	
	return TrackingStateManager->GetTrackingState();
}

FMRSessionInfo UMRBitmapMapper::GetTrackingSessionInfo() const
{
	if (!TrackingStateManager)
	{
		return FMRSessionInfo();
	}
	
	return TrackingStateManager->GetSessionInfo();
}

bool UMRBitmapMapper::IsTrackingStable() const
{
	if (!TrackingStateManager)
	{
		return false;
	}
	
	return TrackingStateManager->IsTrackingStable();
}

void UMRBitmapMapper::InitializeComponents()
{
	// Create specialized components
	Storage = NewObject<UBitmapPointStorage>(this);
	MemoryManager = NewObject<UBitmapPointMemoryManager>(this);
	SpatialIndex = NewObject<UBitmapPointSpatialIndex>(this);
	
	// Get or create tracking state manager (it's a subsystem)
	TrackingStateManager = GetGameInstance()->GetSubsystem<UMRTrackingStateManager>();
	
	// Initialize components
	if (Storage)
	{
		Storage->OnBitmapPointsChanged.AddDynamic(this, &UMRBitmapMapper::OnStoragePointsChanged);
	}
	
	if (MemoryManager && Storage)
	{
		MemoryManager->Initialize(Storage);
		MemoryManager->OnMemoryCleanup.AddDynamic(this, &UMRBitmapMapper::OnMemoryCleanup);
	}
	
	if (SpatialIndex)
	{
		SpatialIndex->Initialize();
		SpatialIndex->OnSpatialIndexUpdated.AddDynamic(this, &UMRBitmapMapper::OnSpatialIndexUpdated);
	}
	
	// Initialize tracking
	LastPlaneDetectionTime = FPlatformTime::Seconds();
}

void UMRBitmapMapper::OnStoragePointsChanged(const TArray<FBitmapPoint>& Points)
{
	BroadcastUpdate();
}

void UMRBitmapMapper::OnMemoryCleanup(int32 PointsRemoved, int32 MemoryFreedKB)
{
	UE_LOG(LogTemp, Verbose, TEXT("MRBitmapMapper: Memory cleanup removed %d points, freed %d KB"), 
		PointsRemoved, MemoryFreedKB);
	
	// Sync spatial index with storage after cleanup
	if (SpatialIndex && Storage && PointsRemoved > 0)
	{
		SpatialIndex->Clear();
		SpatialIndex->AddPoints(Storage->GetAllPoints());
	}
}

void UMRBitmapMapper::OnSpatialIndexUpdated(int32 AddedPoints, int32 RemovedPoints)
{
	// Spatial index updated - could trigger additional processing here if needed
}

void UMRBitmapMapper::BroadcastUpdate()
{
	if (bRealTimeUpdatesEnabled && Storage)
	{
		OnBitmapPointsUpdated.Broadcast(Storage->GetAllPoints());
	}
}

void UMRBitmapMapper::PerformAutoPlaneDetection()
{
	const float CurrentTime = FPlatformTime::Seconds();
	if (CurrentTime - LastPlaneDetectionTime < PlaneDetectionInterval)
	{
		return;
	}
	
	if (!Storage || Storage->GetPointCount() < MinPointsForPlaneDetection)
	{
		return;
	}
	
	// Trigger plane detection
	TArray<FDetectedPlane> DetectedPlanes = DetectPlanesFromCurrentPoints();
	
	if (DetectedPlanes.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("MRBitmapMapper: Auto-detected %d planes"), DetectedPlanes.Num());
	}
	
	LastPlaneDetectionTime = CurrentTime;
}
