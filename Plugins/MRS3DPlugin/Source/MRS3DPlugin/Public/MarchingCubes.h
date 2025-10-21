#pragma once

#include "CoreMinimal.h"
#include "BitmapPoint.h"
#include "MarchingCubes.generated.h"

/**
 * Marching cubes configuration structure
 */
USTRUCT(BlueprintType)
struct FMRS3DPLUGIN_API FMarchingCubesConfig
{
	GENERATED_BODY()

	/** Size of each voxel in the grid */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MarchingCubes")
	float VoxelSize;

	/** Iso-surface value threshold */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MarchingCubes")
	float IsoValue;

	/** Grid bounds for marching cubes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MarchingCubes")
	FVector GridMin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MarchingCubes")
	FVector GridMax;

	/** Resolution of the voxel grid */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MarchingCubes")
	FIntVector GridResolution;

	/** Smoothing factor for generated mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MarchingCubes")
	float SmoothingFactor;

	/** Enable normal smoothing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MarchingCubes")
	bool bSmoothNormals;

	FMarchingCubesConfig()
		: VoxelSize(10.0f)
		, IsoValue(0.5f)
		, GridMin(FVector(-500, -500, -500))
		, GridMax(FVector(500, 500, 500))
		, GridResolution(FIntVector(100, 100, 100))
		, SmoothingFactor(0.5f)
		, bSmoothNormals(true)
	{}
};

/**
 * Voxel data structure for marching cubes
 */
struct FMRS3DPLUGIN_API FVoxel
{
	float Value;
	FVector Position;
	FVector Normal;
	FColor Color;

	FVoxel()
		: Value(0.0f)
		, Position(FVector::ZeroVector)
		, Normal(FVector::UpVector)
		, Color(FColor::White)
	{}

	FVoxel(float InValue, const FVector& InPosition)
		: Value(InValue)
		, Position(InPosition)
		, Normal(FVector::UpVector)
		, Color(FColor::White)
	{}
};

/**
 * Marching cubes triangle structure
 */
struct FMRS3DPLUGIN_API FMCTriangle
{
	FVector Vertices[3];
	FVector Normals[3];
	FColor Colors[3];
	FVector2D UVs[3];

	FMCTriangle()
	{
		for (int32 i = 0; i < 3; i++)
		{
			Vertices[i] = FVector::ZeroVector;
			Normals[i] = FVector::UpVector;
			Colors[i] = FColor::White;
			UVs[i] = FVector2D::ZeroVector;
		}
	}
};

/**
 * Marching cubes algorithm implementation
 */
class FMRS3DPLUGIN_API FMarchingCubesGenerator
{
public:
	FMarchingCubesGenerator();
	~FMarchingCubesGenerator();

	/**
	 * Generate mesh from bitmap points using marching cubes
	 */
	TArray<FMCTriangle> GenerateFromBitmapPoints(const TArray<FBitmapPoint>& Points, const FMarchingCubesConfig& Config);

	/**
	 * Generate mesh from voxel grid
	 */
	TArray<FMCTriangle> GenerateFromVoxelGrid(const TArray<FVoxel>& VoxelGrid, const FMarchingCubesConfig& Config);

	/**
	 * Create voxel grid from bitmap points
	 */
	TArray<FVoxel> CreateVoxelGrid(const TArray<FBitmapPoint>& Points, const FMarchingCubesConfig& Config);

	/**
	 * Get marching cubes lookup tables
	 */
	static const int32 EdgeTable[256];
	static const int32 TriTable[256][16];

private:
	/**
	 * Process a single cube in the marching cubes algorithm
	 */
	TArray<FMCTriangle> ProcessCube(const FVoxel Cube[8], const FMarchingCubesConfig& Config);

	/**
	 * Interpolate vertex position along an edge
	 */
	FVector InterpolateVertex(const FVoxel& V1, const FVoxel& V2, float IsoValue);

	/**
	 * Interpolate vertex color along an edge
	 */
	FColor InterpolateColor(const FVoxel& V1, const FVoxel& V2, float IsoValue);

	/**
	 * Calculate normal vector for a vertex
	 */
	FVector CalculateNormal(const FVector& Position, const TArray<FVoxel>& VoxelGrid, const FMarchingCubesConfig& Config);

	/**
	 * Calculate density value at a position from bitmap points
	 */
	float CalculateDensity(const FVector& Position, const TArray<FBitmapPoint>& Points, float Radius = 50.0f);

	/**
	 * Get voxel index from 3D coordinates
	 */
	int32 GetVoxelIndex(int32 X, int32 Y, int32 Z, const FIntVector& GridResolution);

	/**
	 * Get voxel from grid by coordinates
	 */
	FVoxel GetVoxel(int32 X, int32 Y, int32 Z, const TArray<FVoxel>& VoxelGrid, const FIntVector& GridResolution);

	/**
	 * Smooth normals across the mesh
	 */
	void SmoothNormals(TArray<FMCTriangle>& Triangles, float SmoothingFactor);
};