#include "BitmapPointMemoryManager.h"
#include "Engine/Engine.h"

UBitmapPointMemoryManager::UBitmapPointMemoryManager()
	: Storage(nullptr)
	, MaxBitmapPoints(50000)
	, MaxPointAgeSeconds(300.0f)
	, bAutoCleanupEnabled(true)
	, CleanupIntervalSeconds(30.0f)
	, LastCleanupTime(0.0f)
	, CleanupCount(0)
	, TotalPointsRemoved(0)
	, TotalMemoryFreed(0)
{
}

void UBitmapPointMemoryManager::Initialize(UBitmapPointStorage* InStorage)
{
	Storage = InStorage;
	LastCleanupTime = FPlatformTime::Seconds();
}

void UBitmapPointMemoryManager::SetMaxPoints(int32 MaxPoints)
{
	MaxBitmapPoints = FMath::Max(0, MaxPoints);
	UE_LOG(LogTemp, Log, TEXT("Memory Manager: Max points set to %d"), MaxBitmapPoints);
}

void UBitmapPointMemoryManager::SetMaxPointAge(float MaxAgeSeconds)
{
	MaxPointAgeSeconds = FMath::Max(0.0f, MaxAgeSeconds);
	UE_LOG(LogTemp, Log, TEXT("Memory Manager: Max point age set to %.1f seconds"), MaxPointAgeSeconds);
}

void UBitmapPointMemoryManager::SetAutoCleanupEnabled(bool bEnabled)
{
	bAutoCleanupEnabled = bEnabled;
	UE_LOG(LogTemp, Log, TEXT("Memory Manager: Auto cleanup %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void UBitmapPointMemoryManager::SetCleanupInterval(float IntervalSeconds)
{
	CleanupIntervalSeconds = FMath::Max(1.0f, IntervalSeconds);
	UE_LOG(LogTemp, Log, TEXT("Memory Manager: Cleanup interval set to %.1f seconds"), CleanupIntervalSeconds);
}

int32 UBitmapPointMemoryManager::PerformCleanup()
{
	if (!Storage)
	{
		UE_LOG(LogTemp, Warning, TEXT("Memory Manager: No storage assigned for cleanup"));
		return 0;
	}

	return PerformCleanupInternal();
}

int32 UBitmapPointMemoryManager::RemoveOldPoints()
{
	if (!Storage || MaxPointAgeSeconds <= 0.0f)
	{
		return 0;
	}

	const float CurrentTime = FPlatformTime::Seconds();
	const float OldestAllowedTime = CurrentTime - MaxPointAgeSeconds;

	int32 RemovedCount = Storage->RemovePointsWhere([OldestAllowedTime](const FBitmapPoint& Point) {
		return Point.Timestamp < OldestAllowedTime;
	});

	if (RemovedCount > 0)
	{
		UE_LOG(LogTemp, Verbose, TEXT("Memory Manager: Removed %d old points"), RemovedCount);
	}

	return RemovedCount;
}

int32 UBitmapPointMemoryManager::RemoveExcessPoints()
{
	if (!Storage || MaxBitmapPoints <= 0)
	{
		return 0;
	}

	const int32 CurrentCount = Storage->GetPointCount();
	if (CurrentCount <= MaxBitmapPoints)
	{
		return 0;
	}

	const int32 ExcessCount = CurrentCount - MaxBitmapPoints;

	// Remove oldest points first (FIFO strategy)
	int32 RemovedCount = 0;
	for (int32 i = 0; i < ExcessCount && Storage->GetPointCount() > 0; i++)
	{
		if (Storage->RemovePoint(0)) // Remove first (oldest) point
		{
			RemovedCount++;
		}
	}

	if (RemovedCount > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Memory Manager: Removed %d excess points to stay within limit of %d"), 
			RemovedCount, MaxBitmapPoints);
	}

	return RemovedCount;
}

bool UBitmapPointMemoryManager::ShouldPerformCleanup() const
{
	if (!bAutoCleanupEnabled || !Storage)
	{
		return false;
	}

	const float CurrentTime = FPlatformTime::Seconds();
	return (CurrentTime - LastCleanupTime) >= CleanupIntervalSeconds;
}

int32 UBitmapPointMemoryManager::GetMemoryUsageKB() const
{
	if (!Storage)
	{
		return 0;
	}

	return Storage->GetMemoryUsageBytes() / 1024;
}

void UBitmapPointMemoryManager::GetCleanupStats(int32& TotalCleanupsPerformed, int32& TotalPointsRemoved, int32& TotalMemoryFreedKB) const
{
	TotalCleanupsPerformed = CleanupCount;
	TotalPointsRemoved = this->TotalPointsRemoved;
	TotalMemoryFreedKB = TotalMemoryFreed;
}

void UBitmapPointMemoryManager::Tick(float DeltaTime)
{
	if (ShouldPerformCleanup())
	{
		PerformCleanupInternal();
	}
}

int32 UBitmapPointMemoryManager::PerformCleanupInternal()
{
	const int32 InitialMemoryKB = GetMemoryUsageKB();
	
	// Remove old points first
	int32 OldPointsRemoved = RemoveOldPoints();
	
	// Then remove excess points if still needed
	int32 ExcessPointsRemoved = RemoveExcessPoints();
	
	const int32 TotalRemoved = OldPointsRemoved + ExcessPointsRemoved;
	
	if (TotalRemoved > 0)
	{
		const int32 FinalMemoryKB = GetMemoryUsageKB();
		const int32 MemoryFreed = InitialMemoryKB - FinalMemoryKB;
		
		// Update statistics
		CleanupCount++;
		TotalPointsRemoved += TotalRemoved;
		TotalMemoryFreed += MemoryFreed;
		
		// Shrink storage to reclaim memory
		Storage->Shrink();
		
		UE_LOG(LogTemp, Log, TEXT("Memory Manager: Cleanup completed - removed %d points, freed %d KB"), 
			TotalRemoved, MemoryFreed);
		
		OnMemoryCleanup.Broadcast(TotalRemoved, MemoryFreed);
	}
	
	LastCleanupTime = FPlatformTime::Seconds();
	return TotalRemoved;
}