#include "MeshGenerationManager.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFilemanager.h"

UMeshGenerationManager::UMeshGenerationManager()
	: MaxWorkerThreads(FMath::Max(1, FPlatformMisc::NumberOfCoresIncludingHyperthreads() - 1))
	, bAutoCleanupEnabled(true)
	, AutoCleanupDelaySeconds(30.0f)
	, MaxQueuedJobs(10)
	, LastCleanupTime(0.0f)
{
	NextJobID.Set(1);
}

void UMeshGenerationManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	LastCleanupTime = FPlatformTime::Seconds();
	
	UE_LOG(LogTemp, Log, TEXT("MeshGenerationManager: Initialized with %d max worker threads"), MaxWorkerThreads);
}

void UMeshGenerationManager::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("MeshGenerationManager: Shutting down - cancelling all jobs"));
	
	CancelAllJobs();
	
	// Wait a brief moment for threads to finish
	FPlatformProcess::Sleep(0.1f);
	
	// Force cleanup
	{
		FScopeLock Lock(&JobsMutex);
		ActiveJobs.Empty();
		CompletedJobs.Empty();
	}
	
	Super::Deinitialize();
}

int32 UMeshGenerationManager::SubmitMeshGenerationJob(
	const TArray<FBitmapPoint>& Points,
	EMeshGenerationTaskType TaskType,
	const FMarchingCubesConfig& MarchingCubesConfig,
	float VoxelSize,
	const FOnMeshGenerationComplete& CompletionCallback)
{
	// Check if we can start a new job
	if (!CanStartNewJob())
	{
		UE_LOG(LogTemp, Warning, TEXT("MeshGenerationManager: Cannot start new job - too many active jobs"));
		return -1;
	}

	// Validate input
	if (Points.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("MeshGenerationManager: Cannot submit job with no points"));
		return -1;
	}

	// Create job
	TSharedPtr<FMeshGenerationJob> Job = MakeShared<FMeshGenerationJob>();
	Job->JobID = GenerateJobID();
	Job->CompletionCallback = CompletionCallback;

	// Initialize job info
	Job->Info.JobID = Job->JobID;
	Job->Info.TaskType = TaskType;
	Job->Info.Status = EMeshGenerationTaskStatus::Pending;
	Job->Info.Progress = 0.0f;
	Job->Info.InputPointCount = Points.Num();
	Job->Info.SubmissionTime = FPlatformTime::Seconds();

	// Create task
	Job->Task = MakeShared<FMeshGenerationTask>(Points, TaskType, MarchingCubesConfig, VoxelSize);
	
	// Set completion callback to handle result
	Job->Task->SetCompletionCallback(FOnMeshGenerationComplete::CreateUObject(
		this, &UMeshGenerationManager::OnJobCompleted, Job->JobID));

	// Create and start thread
	FString ThreadName = FString::Printf(TEXT("MeshGen_%d"), Job->JobID);
	Job->Thread = FRunnableThread::Create(Job->Task.Get(), *ThreadName, 0, TPri_BelowNormal);

	if (!Job->Thread)
	{
		UE_LOG(LogTemp, Error, TEXT("MeshGenerationManager: Failed to create thread for job %d"), Job->JobID);
		return -1;
	}

	// Add to active jobs
	{
		FScopeLock Lock(&JobsMutex);
		ActiveJobs.Add(Job->JobID, Job);
	}

	UE_LOG(LogTemp, Log, TEXT("MeshGenerationManager: Started job %d (Type: %d, Points: %d)"),
		Job->JobID, static_cast<int32>(TaskType), Points.Num());

	return Job->JobID;
}

bool UMeshGenerationManager::CancelJob(int32 JobID)
{
	FScopeLock Lock(&JobsMutex);
	
	TSharedPtr<FMeshGenerationJob> Job = ActiveJobs.FindRef(JobID);
	if (!Job)
	{
		return false;
	}

	// Cancel the task
	if (Job->Task)
	{
		Job->Task->Cancel();
	}

	// Update status
	Job->Info.Status = EMeshGenerationTaskStatus::Cancelled;
	Job->Info.CompletionTime = FPlatformTime::Seconds();

	UE_LOG(LogTemp, Log, TEXT("MeshGenerationManager: Cancelled job %d"), JobID);
	return true;
}

bool UMeshGenerationManager::GetJobInfo(int32 JobID, FMeshGenerationJobInfo& OutJobInfo) const
{
	TSharedPtr<FMeshGenerationJob> Job = GetJob(JobID);
	if (!Job)
	{
		return false;
	}

	// Update progress from task
	if (Job->Task)
	{
		Job->Info.Progress = Job->Task->GetProgress() / 100.0f;
		Job->Info.Status = Job->Task->GetStatus();
	}

	OutJobInfo = Job->Info;
	return true;
}

TArray<FMeshGenerationJobInfo> UMeshGenerationManager::GetActiveJobs() const
{
	TArray<FMeshGenerationJobInfo> JobInfos;
	
	FScopeLock Lock(&JobsMutex);
	
	for (const auto& JobPair : ActiveJobs)
	{
		const TSharedPtr<FMeshGenerationJob>& Job = JobPair.Value;
		FMeshGenerationJobInfo Info = Job->Info;
		
		// Update progress from task
		if (Job->Task)
		{
			Info.Progress = Job->Task->GetProgress() / 100.0f;
			Info.Status = Job->Task->GetStatus();
		}
		
		JobInfos.Add(Info);
	}
	
	return JobInfos;
}

bool UMeshGenerationManager::GetJobResult(int32 JobID, FMeshGenerationResult& OutResult) const
{
	TSharedPtr<FMeshGenerationJob> Job = GetJob(JobID);
	if (!Job || !Job->bResultReady)
	{
		return false;
	}

	OutResult = Job->Result;
	return true;
}

void UMeshGenerationManager::CancelAllJobs()
{
	FScopeLock Lock(&JobsMutex);
	
	for (auto& JobPair : ActiveJobs)
	{
		TSharedPtr<FMeshGenerationJob> Job = JobPair.Value;
		if (Job && Job->Task)
		{
			Job->Task->Cancel();
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("MeshGenerationManager: Cancelled all %d active jobs"), ActiveJobs.Num());
}

int32 UMeshGenerationManager::GetActiveThreadCount() const
{
	FScopeLock Lock(&JobsMutex);
	
	int32 ActiveCount = 0;
	for (const auto& JobPair : ActiveJobs)
	{
		const TSharedPtr<FMeshGenerationJob>& Job = JobPair.Value;
		if (Job && Job->Task && Job->Task->GetStatus() == EMeshGenerationTaskStatus::Running)
		{
			ActiveCount++;
		}
	}
	
	return ActiveCount;
}

void UMeshGenerationManager::SetMaxThreadCount(int32 NewMaxThreads)
{
	MaxWorkerThreads = FMath::Clamp(NewMaxThreads, 1, 8);
	UE_LOG(LogTemp, Log, TEXT("MeshGenerationManager: Max worker threads set to %d"), MaxWorkerThreads);
}

int32 UMeshGenerationManager::GetTotalMemoryUsageKB() const
{
	int32 TotalMemory = 0;
	
	FScopeLock Lock(&JobsMutex);
	
	for (const auto& JobPair : ActiveJobs)
	{
		const TSharedPtr<FMeshGenerationJob>& Job = JobPair.Value;
		if (Job && Job->bResultReady)
		{
			TotalMemory += Job->Result.MemoryUsageKB;
		}
	}
	
	for (const auto& JobPair : CompletedJobs)
	{
		const TSharedPtr<FMeshGenerationJob>& Job = JobPair.Value;
		if (Job)
		{
			TotalMemory += Job->Result.MemoryUsageKB;
		}
	}
	
	return TotalMemory;
}

void UMeshGenerationManager::SetAutoCleanupEnabled(bool bEnabled, float CleanupDelaySeconds)
{
	bAutoCleanupEnabled = bEnabled;
	AutoCleanupDelaySeconds = FMath::Max(1.0f, CleanupDelaySeconds);
	
	UE_LOG(LogTemp, Log, TEXT("MeshGenerationManager: Auto cleanup %s (delay: %.1fs)"),
		bEnabled ? TEXT("enabled") : TEXT("disabled"), AutoCleanupDelaySeconds);
}

int32 UMeshGenerationManager::GenerateJobID()
{
	return NextJobID.Increment();
}

void UMeshGenerationManager::OnJobCompleted(int32 JobID, bool bSuccess, const FMeshGenerationResult& Result)
{
	TSharedPtr<FMeshGenerationJob> Job;
	
	// Find and update job
	{
		FScopeLock Lock(&JobsMutex);
		Job = ActiveJobs.FindRef(JobID);
		if (!Job)
		{
			return;
		}

		// Update job info
		Job->Info.Status = bSuccess ? EMeshGenerationTaskStatus::Completed : EMeshGenerationTaskStatus::Failed;
		Job->Info.Progress = 1.0f;
		Job->Info.CompletionTime = FPlatformTime::Seconds();
		Job->Result = Result;
		Job->bResultReady = true;

		// Move to completed jobs
		ActiveJobs.Remove(JobID);
		CompletedJobs.Add(JobID, Job);
	}

	// Execute completion callback
	if (Job->CompletionCallback.IsBound())
	{
		Job->CompletionCallback.ExecuteIfBound(bSuccess, Result);
	}

	// Broadcast event
	OnJobComplete.Broadcast(JobID, bSuccess);

	UE_LOG(LogTemp, Log, TEXT("MeshGenerationManager: Job %d completed %s (%.3fs, %d triangles)"),
		JobID, bSuccess ? TEXT("successfully") : TEXT("with failure"),
		Result.ExecutionTime, Result.TriangleCount);

	// Cleanup if auto cleanup is enabled
	if (bAutoCleanupEnabled)
	{
		CleanupCompletedJobs();
	}
}

void UMeshGenerationManager::CleanupCompletedJobs()
{
	const float CurrentTime = FPlatformTime::Seconds();
	
	if (CurrentTime - LastCleanupTime < AutoCleanupDelaySeconds)
	{
		return;
	}

	FScopeLock Lock(&JobsMutex);
	
	TArray<int32> JobsToRemove;
	
	for (const auto& JobPair : CompletedJobs)
	{
		const TSharedPtr<FMeshGenerationJob>& Job = JobPair.Value;
		if (Job && (CurrentTime - Job->Info.CompletionTime) > AutoCleanupDelaySeconds)
		{
			JobsToRemove.Add(JobPair.Key);
		}
	}

	for (int32 JobID : JobsToRemove)
	{
		CompletedJobs.Remove(JobID);
	}

	if (JobsToRemove.Num() > 0)
	{
		UE_LOG(LogTemp, Verbose, TEXT("MeshGenerationManager: Cleaned up %d completed jobs"), JobsToRemove.Num());
	}

	LastCleanupTime = CurrentTime;
}

bool UMeshGenerationManager::CanStartNewJob() const
{
	FScopeLock Lock(&JobsMutex);
	
	int32 RunningJobs = 0;
	for (const auto& JobPair : ActiveJobs)
	{
		const TSharedPtr<FMeshGenerationJob>& Job = JobPair.Value;
		if (Job && Job->Task && Job->Task->GetStatus() == EMeshGenerationTaskStatus::Running)
		{
			RunningJobs++;
		}
	}

	return RunningJobs < MaxWorkerThreads && ActiveJobs.Num() < MaxQueuedJobs;
}

TSharedPtr<UMeshGenerationManager::FMeshGenerationJob> UMeshGenerationManager::GetJob(int32 JobID) const
{
	FScopeLock Lock(&JobsMutex);
	
	TSharedPtr<FMeshGenerationJob> Job = ActiveJobs.FindRef(JobID);
	if (!Job)
	{
		Job = CompletedJobs.FindRef(JobID);
	}
	
	return Job;
}