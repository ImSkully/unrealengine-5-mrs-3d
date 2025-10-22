#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HAL/RunnableThread.h"
#include "MeshGenerationTask.h"
#include "MeshGenerationManager.generated.h"

USTRUCT(BlueprintType)
struct FMRS3DPLUGIN_API FMeshGenerationJobInfo
{
	GENERATED_BODY()

	/** Unique job ID */
	UPROPERTY(BlueprintReadOnly, Category = "Job")
	int32 JobID;

	/** Task type */
	UPROPERTY(BlueprintReadOnly, Category = "Job")
	EMeshGenerationTaskType TaskType;

	/** Current status */
	UPROPERTY(BlueprintReadOnly, Category = "Job")
	EMeshGenerationTaskStatus Status;

	/** Job progress (0.0 to 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "Job")
	float Progress;

	/** Number of input points */
	UPROPERTY(BlueprintReadOnly, Category = "Job")
	int32 InputPointCount;

	/** Job submission time */
	UPROPERTY(BlueprintReadOnly, Category = "Job")
	float SubmissionTime;

	/** Job completion time (if completed) */
	UPROPERTY(BlueprintReadOnly, Category = "Job")
	float CompletionTime;

	FMeshGenerationJobInfo()
		: JobID(-1)
		, TaskType(EMeshGenerationTaskType::Mesh)
		, Status(EMeshGenerationTaskStatus::Pending)
		, Progress(0.0f)
		, InputPointCount(0)
		, SubmissionTime(0.0f)
		, CompletionTime(0.0f)
	{}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMeshGenerationJobComplete, int32, JobID, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMeshGenerationProgress, int32, JobID, float, Progress);

/**
 * Manages mesh generation worker threads for heavy computation
 * Provides job queuing, progress tracking, and result management
 */
UCLASS(BlueprintType)
class FMRS3DPLUGIN_API UMeshGenerationManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UMeshGenerationManager();

	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem Interface

	/**
	 * Submit a mesh generation job to worker thread
	 * @param Points - Input bitmap points
	 * @param TaskType - Type of mesh generation
	 * @param MarchingCubesConfig - Configuration for marching cubes (if applicable)
	 * @param VoxelSize - Voxel size for voxel-based generation
	 * @param CompletionCallback - Callback for when job completes
	 * @return Job ID for tracking, or -1 if failed to submit
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MeshGeneration")
	int32 SubmitMeshGenerationJob(
		const TArray<FBitmapPoint>& Points,
		EMeshGenerationTaskType TaskType,
		const FMarchingCubesConfig& MarchingCubesConfig = FMarchingCubesConfig(),
		float VoxelSize = 10.0f,
		const FOnMeshGenerationComplete& CompletionCallback = FOnMeshGenerationComplete()
	);

	/**
	 * Cancel a mesh generation job
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MeshGeneration")
	bool CancelJob(int32 JobID);

	/**
	 * Get job information
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MeshGeneration")
	bool GetJobInfo(int32 JobID, FMeshGenerationJobInfo& OutJobInfo) const;

	/**
	 * Get all active jobs
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MeshGeneration")
	TArray<FMeshGenerationJobInfo> GetActiveJobs() const;

	/**
	 * Get mesh generation result for completed job
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MeshGeneration")
	bool GetJobResult(int32 JobID, FMeshGenerationResult& OutResult) const;

	/**
	 * Cancel all active jobs
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MeshGeneration")
	void CancelAllJobs();

	/**
	 * Get number of active worker threads
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MeshGeneration")
	int32 GetActiveThreadCount() const;

	/**
	 * Get maximum number of worker threads
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MeshGeneration")
	int32 GetMaxThreadCount() const { return MaxWorkerThreads; }

	/**
	 * Set maximum number of worker threads
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MeshGeneration")
	void SetMaxThreadCount(int32 NewMaxThreads);

	/**
	 * Get memory usage of all jobs in KB
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MeshGeneration")
	int32 GetTotalMemoryUsageKB() const;

	/**
	 * Set automatic cleanup of completed jobs
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MeshGeneration")
	void SetAutoCleanupEnabled(bool bEnabled, float CleanupDelaySeconds = 30.0f);

	/** Events */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMeshGenerationJobComplete OnJobComplete;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMeshGenerationProgress OnJobProgress;

protected:
	/** Configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	int32 MaxWorkerThreads;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	bool bAutoCleanupEnabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float AutoCleanupDelaySeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	int32 MaxQueuedJobs;

private:
	/** Job tracking */
	struct FMeshGenerationJob
	{
		int32 JobID;
		TSharedPtr<FMeshGenerationTask> Task;
		FRunnableThread* Thread;
		FMeshGenerationJobInfo Info;
		FMeshGenerationResult Result;
		FOnMeshGenerationComplete CompletionCallback;
		bool bResultReady;

		FMeshGenerationJob()
			: JobID(-1)
			, Task(nullptr)
			, Thread(nullptr)
			, bResultReady(false)
		{}

		~FMeshGenerationJob()
		{
			if (Thread)
			{
				Thread->Kill(true);
				delete Thread;
				Thread = nullptr;
			}
		}
	};

	/** Active jobs */
	TMap<int32, TSharedPtr<FMeshGenerationJob>> ActiveJobs;

	/** Completed jobs awaiting cleanup */
	TMap<int32, TSharedPtr<FMeshGenerationJob>> CompletedJobs;

	/** Job ID counter */
	FThreadSafeCounter NextJobID;

	/** Thread synchronization */
	mutable FCriticalSection JobsMutex;

	/** Cleanup timer */
	float LastCleanupTime;

	/** Generate unique job ID */
	int32 GenerateJobID();

	/** Handle job completion */
	void OnJobCompleted(int32 JobID, bool bSuccess, const FMeshGenerationResult& Result);

	/** Cleanup completed jobs */
	void CleanupCompletedJobs();

	/** Update job progress */
	void UpdateJobProgress(int32 JobID);

	/** Check if we can start new job */
	bool CanStartNewJob() const;

	/** Get job by ID (thread-safe) */
	TSharedPtr<FMeshGenerationJob> GetJob(int32 JobID) const;

	/** Internal job status update */
	void UpdateJobStatus();
};