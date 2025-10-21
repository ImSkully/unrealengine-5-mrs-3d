#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralMeshComponent.h"
#include "BitmapPoint.h"
#include "MarchingCubes.h"
#include "ProceduralGenerator.generated.h"

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

protected:
	UPROPERTY()
	UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY()
	TArray<FBitmapPoint> CachedPoints;

	// Marching cubes generator
	FMarchingCubesGenerator* MarchingCubesGenerator;

	float TimeSinceLastUpdate;

	void GeneratePointCloud(const TArray<FBitmapPoint>& Points);
	void GenerateMesh(const TArray<FBitmapPoint>& Points);
	void GenerateVoxels(const TArray<FBitmapPoint>& Points);
	void GenerateSurface(const TArray<FBitmapPoint>& Points);
	void GenerateMarchingCubesInternal(const TArray<FBitmapPoint>& Points);
	
	void CreateProceduralMeshIfNeeded();
	void ConvertMCTrianglesToMesh(const TArray<FMCTriangle>& MCTriangles);
};
