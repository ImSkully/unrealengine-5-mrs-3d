#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralMeshComponent.h"
#include "BitmapPoint.h"
#include "MarchingCubes.h"
#include "MeshGenerationTask.h"
#include "ProceduralGenerator.generated.h"

class UMeshGenerationManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAsyncMeshGenerationComplete, bool, bSuccess, int32, JobID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAsyncMeshProgress, int32, JobID, float, Progress);

UENUM(BlueprintType)
enum class EProceduralGenerationType : uint8
{
	PointCloud UMETA(DisplayName = "Point Cloud"),
	Mesh UMETA(DisplayName = "Mesh"),
	Voxel UMETA(DisplayName = "Voxel"),
	Surface UMETA(DisplayName = "Surface"),
	MarchingCubes UMETA(DisplayName = "Marching Cubes")
};

/**
 * Core procedural generation component that creates geometry from bitmap points
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FMRS3DPLUGIN_API UProceduralGenerator : public UActorComponent
{
	GENERATED_BODY()

public:	
	UProceduralGenerator();
	virtual ~UProceduralGenerator();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Generate procedural geometry from bitmap points
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Procedural")
	void GenerateFromBitmapPoints(const TArray<FBitmapPoint>& Points);

	/**
	 * Update existing geometry with new bitmap points
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Procedural")
	void UpdateGeometry(const TArray<FBitmapPoint>& Points);

	/**
	 * Clear all generated geometry
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Procedural")
	void ClearGeometry();

	/**
	 * Set the generation type
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Procedural")
	void SetGenerationType(EProceduralGenerationType NewType);

	/**
	 * Get current memory usage of cached points in KB
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Procedural")
	int32 GetCachedPointsMemoryKB() const;

	/**
	 * Force cleanup of cached geometry and points
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Procedural")
	void ForceMemoryCleanup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Procedural")
	EProceduralGenerationType GenerationType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Procedural")
	float VoxelSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Procedural")
	bool bAutoUpdate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Procedural")
	float UpdateInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Procedural")
	UMaterialInterface* DefaultMaterial;

	// Marching Cubes Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|MarchingCubes")
	FMarchingCubesConfig MarchingCubesConfig;

	// Worker Thread Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|AsyncGeneration")
	int32 AsyncGenerationThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|AsyncGeneration")
	bool bEnableAsyncGeneration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|AsyncGeneration")
	bool bShowAsyncProgress;

	/** Events for async generation */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAsyncMeshGenerationComplete OnAsyncGenerationComplete;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAsyncMeshProgress OnAsyncGenerationProgress;

	/**
	 * Set marching cubes configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MarchingCubes")
	void SetMarchingCubesConfig(const FMarchingCubesConfig& NewConfig);

	/**
	 * Generate mesh using marching cubes algorithm
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MarchingCubes")
	void GenerateMarchingCubes(const TArray<FBitmapPoint>& Points);

	/**
	 * Update marching cubes grid bounds automatically from points
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MarchingCubes")
	void UpdateGridBoundsFromPoints(const TArray<FBitmapPoint>& Points, float Padding = 100.0f);

	/**
	 * Generate mesh asynchronously using worker threads for large datasets
	 * @param Points - Input bitmap points
	 * @param bForceAsync - Force async generation even for small datasets
	 * @return Job ID for tracking, or -1 if generated synchronously
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|AsyncGeneration")
	int32 GenerateAsyncFromBitmapPoints(const TArray<FBitmapPoint>& Points, bool bForceAsync = false);

	/**
	 * Cancel an active async generation job
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|AsyncGeneration")
	bool CancelAsyncGeneration(int32 JobID);

	/**
	 * Get progress of async generation job
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|AsyncGeneration")
	float GetAsyncGenerationProgress(int32 JobID) const;

	/**
	 * Check if async generation is currently running
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|AsyncGeneration")
	bool IsAsyncGenerationActive() const;

	/**
	 * Set threshold for using async generation
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|AsyncGeneration")
	void SetAsyncThreshold(int32 NewThreshold);

	/**
	 * Get current async generation threshold
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|AsyncGeneration")
	int32 GetAsyncThreshold() const { return AsyncGenerationThreshold; }

protected:
	UPROPERTY()
	UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY()
	TArray<FBitmapPoint> CachedPoints;

	// Marching cubes generator
	FMarchingCubesGenerator* MarchingCubesGenerator;

	float TimeSinceLastUpdate;

	// Worker thread management
	UPROPERTY()
	UMeshGenerationManager* MeshGenerationManager;

	TArray<int32> ActiveAsyncJobs;
	mutable FCriticalSection AsyncJobsMutex;

	void GeneratePointCloud(const TArray<FBitmapPoint>& Points);
	void GenerateMesh(const TArray<FBitmapPoint>& Points);
	void GenerateVoxels(const TArray<FBitmapPoint>& Points);
	void GenerateSurface(const TArray<FBitmapPoint>& Points);
	void GenerateMarchingCubesInternal(const TArray<FBitmapPoint>& Points);
	
	void CreateProceduralMeshIfNeeded();
	void ConvertMCTrianglesToMesh(const TArray<FMCTriangle>& MCTriangles);

	// Async generation support
	bool ShouldUseAsyncGeneration(int32 PointCount) const;
	EMeshGenerationTaskType GetTaskTypeFromGenerationType() const;
	UFUNCTION()
	void OnAsyncJobCompleted(int32 JobID, bool bSuccess);
	void ApplyAsyncResult(int32 JobID, const FMeshGenerationResult& Result);
	void CleanupCompletedAsyncJobs();
};
