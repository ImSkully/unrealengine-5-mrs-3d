#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralMeshComponent.h"
#include "BitmapPoint.h"
#include "ProceduralGenerator.generated.h"

UENUM(BlueprintType)
enum class EProceduralGenerationType : uint8
{
	PointCloud UMETA(DisplayName = "Point Cloud"),
	Mesh UMETA(DisplayName = "Mesh"),
	Voxel UMETA(DisplayName = "Voxel"),
	Surface UMETA(DisplayName = "Surface")
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

protected:
	UPROPERTY()
	UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY()
	TArray<FBitmapPoint> CachedPoints;

	float TimeSinceLastUpdate;

	void GeneratePointCloud(const TArray<FBitmapPoint>& Points);
	void GenerateMesh(const TArray<FBitmapPoint>& Points);
	void GenerateVoxels(const TArray<FBitmapPoint>& Points);
	void GenerateSurface(const TArray<FBitmapPoint>& Points);
	
	void CreateProceduralMeshIfNeeded();
};
