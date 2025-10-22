#include "MeshGenerationTask.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFilemanager.h"

FMeshGenerationTask::FMeshGenerationTask(
	const TArray<FBitmapPoint>& InPoints,
	EMeshGenerationTaskType InTaskType,
	const FMarchingCubesConfig& InMarchingCubesConfig,
	float InVoxelSize
)
	: Points(InPoints)
	, TaskType(InTaskType)
	, MarchingCubesConfig(InMarchingCubesConfig)
	, VoxelSize(InVoxelSize)
	, bShouldCancel(false)
{
	Status.Set(static_cast<int32>(EMeshGenerationTaskStatus::Pending));
	Progress.Set(0);
	
	// Create marching cubes generator if needed
	if (TaskType == EMeshGenerationTaskType::MarchingCubes)
	{
		MarchingCubesGenerator = MakeUnique<FMarchingCubesGenerator>();
	}
}

FMeshGenerationTask::~FMeshGenerationTask()
{
}

bool FMeshGenerationTask::Init()
{
	UE_LOG(LogTemp, Log, TEXT("MeshGenerationTask: Initializing task for %d points (Type: %d)"), 
		Points.Num(), static_cast<int32>(TaskType));
	
	Result.InputPointCount = Points.Num();
	SetStatus(EMeshGenerationTaskStatus::Running);
	return true;
}

uint32 FMeshGenerationTask::Run()
{
	const double StartTime = FPlatformTime::Seconds();
	bool bSuccess = false;

	try
	{
		UpdateProgress(0.1f);

		// Check for cancellation before starting heavy work
		if (ShouldCancel())
		{
			SetStatus(EMeshGenerationTaskStatus::Cancelled);
			return 0;
		}

		// Perform mesh generation based on type
		switch (TaskType)
		{
		case EMeshGenerationTaskType::PointCloud:
			bSuccess = GeneratePointCloudMesh();
			break;
		case EMeshGenerationTaskType::Mesh:
			bSuccess = GenerateTriangulatedMesh();
			break;
		case EMeshGenerationTaskType::Voxel:
			bSuccess = GenerateVoxelMesh();
			break;
		case EMeshGenerationTaskType::MarchingCubes:
			bSuccess = GenerateMarchingCubesMesh();
			break;
		default:
			UE_LOG(LogTemp, Error, TEXT("MeshGenerationTask: Unknown task type"));
			bSuccess = false;
			break;
		}

		UpdateProgress(0.9f);

		// Finalize results
		if (bSuccess && !ShouldCancel())
		{
			Result.ExecutionTime = FPlatformTime::Seconds() - StartTime;
			Result.TriangleCount = Result.Triangles.Num() / 3;
			Result.MemoryUsageKB = CalculateMemoryUsage();
			
			SetStatus(EMeshGenerationTaskStatus::Completed);
			LogTaskStats();
		}
		else if (ShouldCancel())
		{
			SetStatus(EMeshGenerationTaskStatus::Cancelled);
		}
		else
		{
			SetStatus(EMeshGenerationTaskStatus::Failed);
		}

		UpdateProgress(1.0f);
	}
	catch (const std::exception& e)
	{
		UE_LOG(LogTemp, Error, TEXT("MeshGenerationTask: Exception during generation: %s"), 
			UTF8_TO_TCHAR(e.what()));
		SetStatus(EMeshGenerationTaskStatus::Failed);
		bSuccess = false;
	}

	// Trigger completion callback on game thread
	if (CompletionCallback.IsBound())
	{
		AsyncTask(ENamedThreads::GameThread, [this, bSuccess]()
		{
			CompletionCallback.ExecuteIfBound(bSuccess, Result);
		});
	}

	return bSuccess ? 1 : 0;
}

void FMeshGenerationTask::Stop()
{
	Cancel();
}

void FMeshGenerationTask::Exit()
{
	UE_LOG(LogTemp, Verbose, TEXT("MeshGenerationTask: Task exiting"));
}

void FMeshGenerationTask::SetCompletionCallback(const FOnMeshGenerationComplete& InCallback)
{
	CompletionCallback = InCallback;
}

void FMeshGenerationTask::Cancel()
{
	bShouldCancel = true;
	UE_LOG(LogTemp, Log, TEXT("MeshGenerationTask: Cancellation requested"));
}

bool FMeshGenerationTask::GeneratePointCloudMesh()
{
	UpdateProgress(0.2f);

	const float PointSize = VoxelSize * 0.5f;
	const int32 VerticesPerPoint = 8;
	const int32 TrianglesPerPoint = 12; // 6 faces * 2 triangles per face

	// Pre-allocate arrays for better performance
	Result.Vertices.Reserve(Points.Num() * VerticesPerPoint);
	Result.Triangles.Reserve(Points.Num() * TrianglesPerPoint * 3);
	Result.Normals.Reserve(Points.Num() * VerticesPerPoint);
	Result.UV0.Reserve(Points.Num() * VerticesPerPoint);
	Result.VertexColors.Reserve(Points.Num() * VerticesPerPoint);

	for (int32 i = 0; i < Points.Num(); i++)
	{
		// Check for cancellation periodically
		if (i % 1000 == 0)
		{
			if (ShouldCancel()) return false;
			UpdateProgress(0.2f + (0.6f * i / Points.Num()));
		}

		const FBitmapPoint& Point = Points[i];
		const int32 BaseVertex = Result.Vertices.Num();
		const FVector HalfSize(PointSize);

		// Add 8 vertices for cube
		Result.Vertices.Add(Point.Position + FVector(-HalfSize.X, -HalfSize.Y, -HalfSize.Z));
		Result.Vertices.Add(Point.Position + FVector(HalfSize.X, -HalfSize.Y, -HalfSize.Z));
		Result.Vertices.Add(Point.Position + FVector(HalfSize.X, HalfSize.Y, -HalfSize.Z));
		Result.Vertices.Add(Point.Position + FVector(-HalfSize.X, HalfSize.Y, -HalfSize.Z));
		Result.Vertices.Add(Point.Position + FVector(-HalfSize.X, -HalfSize.Y, HalfSize.Z));
		Result.Vertices.Add(Point.Position + FVector(HalfSize.X, -HalfSize.Y, HalfSize.Z));
		Result.Vertices.Add(Point.Position + FVector(HalfSize.X, HalfSize.Y, HalfSize.Z));
		Result.Vertices.Add(Point.Position + FVector(-HalfSize.X, HalfSize.Y, HalfSize.Z));

		// Add vertex attributes
		for (int32 j = 0; j < 8; j++)
		{
			Result.VertexColors.Add(Point.Color);
			Result.Normals.Add(Point.Normal);
			Result.UV0.Add(FVector2D(0, 0));
		}

		// Add triangles for cube faces
		TArray<int32> CubeTriangles = {
			// Front face
			BaseVertex + 0, BaseVertex + 1, BaseVertex + 2,
			BaseVertex + 0, BaseVertex + 2, BaseVertex + 3,
			// Back face
			BaseVertex + 5, BaseVertex + 4, BaseVertex + 7,
			BaseVertex + 5, BaseVertex + 7, BaseVertex + 6,
			// Left face
			BaseVertex + 4, BaseVertex + 0, BaseVertex + 3,
			BaseVertex + 4, BaseVertex + 3, BaseVertex + 7,
			// Right face
			BaseVertex + 1, BaseVertex + 5, BaseVertex + 6,
			BaseVertex + 1, BaseVertex + 6, BaseVertex + 2,
			// Top face
			BaseVertex + 3, BaseVertex + 2, BaseVertex + 6,
			BaseVertex + 3, BaseVertex + 6, BaseVertex + 7,
			// Bottom face
			BaseVertex + 4, BaseVertex + 5, BaseVertex + 1,
			BaseVertex + 4, BaseVertex + 1, BaseVertex + 0
		};

		Result.Triangles.Append(CubeTriangles);
	}

	UpdateProgress(0.8f);
	return true;
}

bool FMeshGenerationTask::GenerateTriangulatedMesh()
{
	UpdateProgress(0.2f);

	if (Points.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("MeshGenerationTask: Not enough points for triangulation"));
		return false;
	}

	// Pre-allocate arrays
	Result.Vertices.Reserve(Points.Num());
	Result.Triangles.Reserve((Points.Num() - 2) * 3);
	Result.Normals.Reserve(Points.Num());
	Result.UV0.Reserve(Points.Num());
	Result.VertexColors.Reserve(Points.Num());

	// Add vertices
	for (int32 i = 0; i < Points.Num(); i++)
	{
		if (i % 1000 == 0)
		{
			if (ShouldCancel()) return false;
			UpdateProgress(0.2f + (0.4f * i / Points.Num()));
		}

		const FBitmapPoint& Point = Points[i];
		Result.Vertices.Add(Point.Position);
		Result.Normals.Add(Point.Normal);
		Result.VertexColors.Add(Point.Color);
		Result.UV0.Add(FVector2D(0, 0));
	}

	UpdateProgress(0.6f);

	// Create triangles using fan triangulation
	for (int32 i = 1; i < Points.Num() - 1; i++)
	{
		if (i % 1000 == 0)
		{
			if (ShouldCancel()) return false;
			UpdateProgress(0.6f + (0.2f * i / (Points.Num() - 1)));
		}

		Result.Triangles.Add(0);
		Result.Triangles.Add(i);
		Result.Triangles.Add(i + 1);
	}

	UpdateProgress(0.8f);
	return true;
}

bool FMeshGenerationTask::GenerateVoxelMesh()
{
	UpdateProgress(0.2f);

	// Snap points to voxel grid
	TSet<FIntVector> VoxelSet;
	for (int32 i = 0; i < Points.Num(); i++)
	{
		if (i % 1000 == 0)
		{
			if (ShouldCancel()) return false;
			UpdateProgress(0.2f + (0.2f * i / Points.Num()));
		}

		const FBitmapPoint& Point = Points[i];
		FIntVector VoxelPos(
			FMath::FloorToInt(Point.Position.X / VoxelSize),
			FMath::FloorToInt(Point.Position.Y / VoxelSize),
			FMath::FloorToInt(Point.Position.Z / VoxelSize)
		);
		VoxelSet.Add(VoxelPos);
	}

	UpdateProgress(0.4f);

	// Pre-allocate arrays
	const int32 VerticesPerVoxel = 8;
	const int32 TrianglesPerVoxel = 12;
	Result.Vertices.Reserve(VoxelSet.Num() * VerticesPerVoxel);
	Result.Triangles.Reserve(VoxelSet.Num() * TrianglesPerVoxel * 3);
	Result.Normals.Reserve(VoxelSet.Num() * VerticesPerVoxel);
	Result.UV0.Reserve(VoxelSet.Num() * VerticesPerVoxel);
	Result.VertexColors.Reserve(VoxelSet.Num() * VerticesPerVoxel);

	// Generate mesh for each voxel
	int32 ProcessedVoxels = 0;
	for (const FIntVector& VoxelPos : VoxelSet)
	{
		if (ProcessedVoxels % 100 == 0)
		{
			if (ShouldCancel()) return false;
			UpdateProgress(0.4f + (0.4f * ProcessedVoxels / VoxelSet.Num()));
		}

		const FVector WorldPos(VoxelPos.X * VoxelSize, VoxelPos.Y * VoxelSize, VoxelPos.Z * VoxelSize);
		const int32 BaseVertex = Result.Vertices.Num();
		const FVector HalfSize(VoxelSize * 0.5f);

		// Add 8 vertices for voxel cube
		Result.Vertices.Add(WorldPos + FVector(-HalfSize.X, -HalfSize.Y, -HalfSize.Z));
		Result.Vertices.Add(WorldPos + FVector(HalfSize.X, -HalfSize.Y, -HalfSize.Z));
		Result.Vertices.Add(WorldPos + FVector(HalfSize.X, HalfSize.Y, -HalfSize.Z));
		Result.Vertices.Add(WorldPos + FVector(-HalfSize.X, HalfSize.Y, -HalfSize.Z));
		Result.Vertices.Add(WorldPos + FVector(-HalfSize.X, -HalfSize.Y, HalfSize.Z));
		Result.Vertices.Add(WorldPos + FVector(HalfSize.X, -HalfSize.Y, HalfSize.Z));
		Result.Vertices.Add(WorldPos + FVector(HalfSize.X, HalfSize.Y, HalfSize.Z));
		Result.Vertices.Add(WorldPos + FVector(-HalfSize.X, HalfSize.Y, HalfSize.Z));

		// Add vertex attributes
		for (int32 j = 0; j < 8; j++)
		{
			Result.VertexColors.Add(FColor::White);
			Result.Normals.Add(FVector::UpVector);
			Result.UV0.Add(FVector2D(0, 0));
		}

		// Add cube faces
		TArray<int32> CubeTriangles = {
			BaseVertex + 0, BaseVertex + 1, BaseVertex + 2,
			BaseVertex + 0, BaseVertex + 2, BaseVertex + 3,
			BaseVertex + 5, BaseVertex + 4, BaseVertex + 7,
			BaseVertex + 5, BaseVertex + 7, BaseVertex + 6,
			BaseVertex + 4, BaseVertex + 0, BaseVertex + 3,
			BaseVertex + 4, BaseVertex + 3, BaseVertex + 7,
			BaseVertex + 1, BaseVertex + 5, BaseVertex + 6,
			BaseVertex + 1, BaseVertex + 6, BaseVertex + 2,
			BaseVertex + 3, BaseVertex + 2, BaseVertex + 6,
			BaseVertex + 3, BaseVertex + 6, BaseVertex + 7,
			BaseVertex + 4, BaseVertex + 5, BaseVertex + 1,
			BaseVertex + 4, BaseVertex + 1, BaseVertex + 0
		};

		Result.Triangles.Append(CubeTriangles);
		ProcessedVoxels++;
	}

	UpdateProgress(0.8f);
	return true;
}

bool FMeshGenerationTask::GenerateMarchingCubesMesh()
{
	UpdateProgress(0.2f);

	if (!MarchingCubesGenerator)
	{
		UE_LOG(LogTemp, Error, TEXT("MeshGenerationTask: Marching cubes generator not available"));
		return false;
	}

	if (ShouldCancel()) return false;

	// Generate marching cubes triangles
	UpdateProgress(0.3f);
	TArray<FMCTriangle> MCTriangles = MarchingCubesGenerator->GenerateFromBitmapPoints(Points, MarchingCubesConfig);

	if (ShouldCancel()) return false;
	UpdateProgress(0.7f);

	if (MCTriangles.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("MeshGenerationTask: Marching cubes generated no triangles"));
		return false;
	}

	// Convert marching cubes triangles to mesh format
	Result.Vertices.Reserve(MCTriangles.Num() * 3);
	Result.Triangles.Reserve(MCTriangles.Num() * 3);
	Result.Normals.Reserve(MCTriangles.Num() * 3);
	Result.UV0.Reserve(MCTriangles.Num() * 3);
	Result.VertexColors.Reserve(MCTriangles.Num() * 3);

	for (int32 i = 0; i < MCTriangles.Num(); i++)
	{
		if (i % 1000 == 0)
		{
			if (ShouldCancel()) return false;
			UpdateProgress(0.7f + (0.1f * i / MCTriangles.Num()));
		}

		const FMCTriangle& Triangle = MCTriangles[i];
		const int32 BaseIndex = Result.Vertices.Num();

		// Add vertices
		for (int32 j = 0; j < 3; j++)
		{
			Result.Vertices.Add(Triangle.Vertices[j]);
			Result.Normals.Add(Triangle.Normals[j]);
			Result.UV0.Add(Triangle.UVs[j]);
			Result.VertexColors.Add(Triangle.Colors[j]);
		}

		// Add triangle indices
		Result.Triangles.Add(BaseIndex);
		Result.Triangles.Add(BaseIndex + 1);
		Result.Triangles.Add(BaseIndex + 2);
	}

	UpdateProgress(0.8f);
	return true;
}

void FMeshGenerationTask::UpdateProgress(float NewProgress)
{
	Progress.Set(FMath::FloorToInt(NewProgress * 100.0f));
}

void FMeshGenerationTask::SetStatus(EMeshGenerationTaskStatus NewStatus)
{
	FScopeLock Lock(&StatusMutex);
	Status.Set(static_cast<int32>(NewStatus));
}

int32 FMeshGenerationTask::CalculateMemoryUsage() const
{
	int32 TotalBytes = 0;
	TotalBytes += Result.Vertices.GetAllocatedSize();
	TotalBytes += Result.Triangles.GetAllocatedSize();
	TotalBytes += Result.Normals.GetAllocatedSize();
	TotalBytes += Result.UV0.GetAllocatedSize();
	TotalBytes += Result.VertexColors.GetAllocatedSize();
	TotalBytes += Result.Tangents.GetAllocatedSize();
	return TotalBytes / 1024; // Convert to KB
}

void FMeshGenerationTask::LogTaskStats() const
{
	UE_LOG(LogTemp, Log, TEXT("MeshGenerationTask: Completed - Type: %d, Points: %d, Triangles: %d, Time: %.3fs, Memory: %dKB"),
		static_cast<int32>(TaskType),
		Result.InputPointCount,
		Result.TriangleCount,
		Result.ExecutionTime,
		Result.MemoryUsageKB);
}