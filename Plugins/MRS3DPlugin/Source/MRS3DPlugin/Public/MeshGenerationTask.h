#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "BitmapPoint.h"
#include "MarchingCubes.h"
#include "ProceduralMeshComponent.h"
#include "MeshGenerationTask.generated.h"

UENUM(BlueprintType)
enum class EMeshGenerationTaskType : uint8
{
	PointCloud      UMETA(DisplayName = "Point Cloud"),
	Mesh           UMETA(DisplayName = "Mesh"),
	Voxel          UMETA(DisplayName = "Voxel"),
	MarchingCubes  UMETA(DisplayName = "Marching Cubes")
};

UENUM(BlueprintType)
enum class EMeshGenerationTaskStatus : uint8
{
	Pending        UMETA(DisplayName = "Pending"),
	Running        UMETA(DisplayName = "Running"),
	Completed      UMETA(DisplayName = "Completed"),
	Failed         UMETA(DisplayName = "Failed"),
	Cancelled      UMETA(DisplayName = "Cancelled")
};

USTRUCT(BlueprintType)
struct FMRS3DPLUGIN_API FMeshGenerationResult
{
	GENERATED_BODY()

	/** Generated vertices */
	TArray<FVector> Vertices;

	/** Generated triangles */
	TArray<int32> Triangles;

	/** Generated normals */
	TArray<FVector> Normals;

	/** Generated UVs */
	TArray<FVector2D> UV0;

	/** Generated vertex colors */
	TArray<FColor> VertexColors;

	/** Generated tangents */
	TArray<FProcMeshTangent> Tangents;

	/** Task execution time in seconds */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float ExecutionTime;

	/** Number of input points processed */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 InputPointCount;

	/** Number of triangles generated */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TriangleCount;

	/** Memory usage in KB */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 MemoryUsageKB;

	FMeshGenerationResult()
		: ExecutionTime(0.0f)
		, InputPointCount(0)
		, TriangleCount(0)
		, MemoryUsageKB(0)
	{}
};

DECLARE_DELEGATE_TwoParams(FOnMeshGenerationComplete, bool /*bSuccess*/, const FMeshGenerationResult& /*Result*/);

/**
 * Runnable task for generating meshes on worker threads
 * Handles heavy mesh generation operations without blocking the game thread
 */
class FMRS3DPLUGIN_API FMeshGenerationTask : public FRunnable
{
public:
	FMeshGenerationTask(
		const TArray<FBitmapPoint>& InPoints,
		EMeshGenerationTaskType InTaskType,
		const FMarchingCubesConfig& InMarchingCubesConfig = FMarchingCubesConfig(),
		float InVoxelSize = 10.0f
	);

	virtual ~FMeshGenerationTask();

	//~ Begin FRunnable Interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;
	//~ End FRunnable Interface

	/** Set completion callback */
	void SetCompletionCallback(const FOnMeshGenerationComplete& InCallback);

	/** Get current task status */
	EMeshGenerationTaskStatus GetStatus() const { return Status; }

	/** Get task progress (0.0 to 1.0) */
	float GetProgress() const { return Progress; }

	/** Get result if task is completed */
	const FMeshGenerationResult& GetResult() const { return Result; }

	/** Cancel the task */
	void Cancel();

	/** Check if task should be cancelled */
	bool ShouldCancel() const { return bShouldCancel; }

private:
	/** Input data */
	TArray<FBitmapPoint> Points;
	EMeshGenerationTaskType TaskType;
	FMarchingCubesConfig MarchingCubesConfig;
	float VoxelSize;

	/** Task state */
	FThreadSafeCounter Status;
	FThreadSafeCounter Progress;
	TAtomic<bool> bShouldCancel;

	/** Results */
	FMeshGenerationResult Result;
	FOnMeshGenerationComplete CompletionCallback;

	/** Thread synchronization */
	FCriticalSection StatusMutex;
	FCriticalSection ResultMutex;

	/** Generation methods for different types */
	bool GeneratePointCloudMesh();
	bool GenerateTriangulatedMesh();
	bool GenerateVoxelMesh();
	bool GenerateMarchingCubesMesh();

	/** Helper methods */
	void UpdateProgress(float NewProgress);
	void SetStatus(EMeshGenerationTaskStatus NewStatus);
	int32 CalculateMemoryUsage() const;
	void LogTaskStats() const;

	/** Marching cubes generator for this task */
	TUniquePtr<FMarchingCubesGenerator> MarchingCubesGenerator;
};