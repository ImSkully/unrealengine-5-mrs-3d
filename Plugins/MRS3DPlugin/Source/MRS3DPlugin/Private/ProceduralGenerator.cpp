#include "ProceduralGenerator.h"
#include "MRBitmapMapper.h"
#include "MeshGenerationManager.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"

UProceduralGenerator::UProceduralGenerator()
	: GenerationType(EProceduralGenerationType::Mesh)
	, VoxelSize(10.0f)
	, bAutoUpdate(true)
	, UpdateInterval(0.1f)
	, AsyncGenerationThreshold(10000)
	, bEnableAsyncGeneration(true)
	, bShowAsyncProgress(true)
	, bFreezeMeshOnTrackingLoss(true)
	, bAutoRecoverFromTrackingLoss(true)
	, TrackingQualityThreshold(0.7f)
	, MaxTrackingLossDuration(30.0f)
	, bUseSpatialAnchors(true)
	, TimeSinceLastUpdate(0.0f)
	, MarchingCubesGenerator(nullptr)
	, MeshGenerationManager(nullptr)
	, CurrentTrackingQuality(1.0f)
	, CurrentTrackingState(ETrackingState::FullTracking)
	, TrackingLossStartTime(0.0f)
	, bIsTrackingLost(false)
	, LastKnownAnchorTransform(FTransform::Identity)
	, CurrentAnchorID(TEXT(""))
{
	PrimaryComponentTick.bCanEverTick = true;
	
	// Initialize marching cubes generator
	MarchingCubesGenerator = new FMarchingCubesGenerator();
	
	// Set default marching cubes configuration
	MarchingCubesConfig.VoxelSize = VoxelSize;
	MarchingCubesConfig.GridResolution = FIntVector(50, 50, 50);
}

UProceduralGenerator::~UProceduralGenerator()
{
	// Cancel all active async jobs
	if (MeshGenerationManager)
	{
		FScopeLock Lock(&AsyncJobsMutex);
		for (int32 JobID : ActiveAsyncJobs)
		{
			MeshGenerationManager->CancelJob(JobID);
		}
		ActiveAsyncJobs.Empty();
	}
	
	if (MarchingCubesGenerator)
	{
		delete MarchingCubesGenerator;
		MarchingCubesGenerator = nullptr;
	}
}

void UProceduralGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	CreateProceduralMeshIfNeeded();
	
	// Get mesh generation manager for async operations
	if (GetWorld() && GetWorld()->GetGameInstance())
	{
		MeshGenerationManager = GetWorld()->GetGameInstance()->GetSubsystem<UMeshGenerationManager>();
	}
	
	if (!MeshGenerationManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProceduralGenerator: MeshGenerationManager not available - async generation disabled"));
		bEnableAsyncGeneration = false;
	}
	
	// Subscribe to bitmap mapper updates
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UMRBitmapMapper* Mapper = GameInstance->GetSubsystem<UMRBitmapMapper>())
		{
			Mapper->OnBitmapPointsUpdated.AddDynamic(this, &UProceduralGenerator::UpdateGeometry);
		}
	}
}

void UProceduralGenerator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// Update async job progress and cleanup completed jobs
	if (bEnableAsyncGeneration && MeshGenerationManager)
	{
		CleanupCompletedAsyncJobs();
		
		// Broadcast progress updates if enabled
		if (bShowAsyncProgress)
		{
			FScopeLock Lock(&AsyncJobsMutex);
			for (int32 JobID : ActiveAsyncJobs)
			{
				float Progress = GetAsyncGenerationProgress(JobID);
				OnAsyncGenerationProgress.Broadcast(JobID, Progress);
			}
		}
	}

	if (bAutoUpdate)
	{
		TimeSinceLastUpdate += DeltaTime;
		
		if (TimeSinceLastUpdate >= UpdateInterval)
		{
			TimeSinceLastUpdate = 0.0f;
			
			if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
			{
				if (UMRBitmapMapper* Mapper = GameInstance->GetSubsystem<UMRBitmapMapper>())
				{
					const TArray<FBitmapPoint>& Points = Mapper->GetBitmapPoints();
					if (Points.Num() > 0 && Points != CachedPoints)
					{
						UpdateGeometry(Points);
					}
				}
			}
		}
	}
}

void UProceduralGenerator::GenerateFromBitmapPoints(const TArray<FBitmapPoint>& Points)
{
	// Check if we should use async generation for large datasets
	if (ShouldUseAsyncGeneration(Points.Num()))
	{
		int32 JobID = GenerateAsyncFromBitmapPoints(Points);
		if (JobID != -1)
		{
			UE_LOG(LogTemp, Log, TEXT("ProceduralGenerator: Started async generation (Job %d) for %d points"), JobID, Points.Num());
			return;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ProceduralGenerator: Failed to start async generation, falling back to sync"));
		}
	}
	
	// Memory-aware caching - only cache if reasonable size
	if (Points.Num() <= 100000)
	{
		CachedPoints = Points;
	}
	else
	{
		// For large datasets, clear cache to save memory
		CachedPoints.Empty();
		CachedPoints.Shrink();
		UE_LOG(LogTemp, Warning, TEXT("ProceduralGenerator: Large point cloud not cached to preserve memory"));
	}
	
	switch (GenerationType)
	{
		case EProceduralGenerationType::PointCloud:
			GeneratePointCloud(Points);
			break;
		case EProceduralGenerationType::Mesh:
			GenerateMesh(Points);
			break;
		case EProceduralGenerationType::Voxel:
			GenerateVoxels(Points);
			break;
		case EProceduralGenerationType::Surface:
			GenerateSurface(Points);
			break;
		case EProceduralGenerationType::MarchingCubes:
			GenerateMarchingCubesInternal(Points);
			break;
	}
}

void UProceduralGenerator::UpdateGeometry(const TArray<FBitmapPoint>& Points)
{
	// Memory-aware update - limit cached points for performance
	if (Points.Num() > 100000) // Prevent excessive memory usage
	{
		UE_LOG(LogTemp, Warning, TEXT("ProceduralGenerator: Point cloud too large (%d points), performance may suffer"), Points.Num());
	}
	
	GenerateFromBitmapPoints(Points);
}

void UProceduralGenerator::ClearGeometry()
{
	if (ProceduralMesh)
	{
		ProceduralMesh->ClearAllMeshSections();
	}
	
	// Clear cached points and shrink array to free memory
	CachedPoints.Empty();
	CachedPoints.Shrink();
}

void UProceduralGenerator::SetGenerationType(EProceduralGenerationType NewType)
{
	GenerationType = NewType;
	if (CachedPoints.Num() > 0)
	{
		GenerateFromBitmapPoints(CachedPoints);
	}
}

void UProceduralGenerator::CreateProceduralMeshIfNeeded()
{
	if (!ProceduralMesh)
	{
		ProceduralMesh = NewObject<UProceduralMeshComponent>(GetOwner(), TEXT("ProceduralMesh"));
		ProceduralMesh->RegisterComponent();
		ProceduralMesh->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		
		if (DefaultMaterial)
		{
			ProceduralMesh->SetMaterial(0, DefaultMaterial);
		}
	}
}

void UProceduralGenerator::GeneratePointCloud(const TArray<FBitmapPoint>& Points)
{
	CreateProceduralMeshIfNeeded();
	ProceduralMesh->ClearAllMeshSections();

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// Create small cubes for each point
	const float PointSize = VoxelSize * 0.5f;
	
	for (int32 i = 0; i < Points.Num(); i++)
	{
		const FBitmapPoint& Point = Points[i];
		int32 BaseVertex = Vertices.Num();
		
		// Create a small cube at the point position
		FVector HalfSize(PointSize);
		
		// Add 8 vertices for cube
		Vertices.Add(Point.Position + FVector(-HalfSize.X, -HalfSize.Y, -HalfSize.Z));
		Vertices.Add(Point.Position + FVector(HalfSize.X, -HalfSize.Y, -HalfSize.Z));
		Vertices.Add(Point.Position + FVector(HalfSize.X, HalfSize.Y, -HalfSize.Z));
		Vertices.Add(Point.Position + FVector(-HalfSize.X, HalfSize.Y, -HalfSize.Z));
		Vertices.Add(Point.Position + FVector(-HalfSize.X, -HalfSize.Y, HalfSize.Z));
		Vertices.Add(Point.Position + FVector(HalfSize.X, -HalfSize.Y, HalfSize.Z));
		Vertices.Add(Point.Position + FVector(HalfSize.X, HalfSize.Y, HalfSize.Z));
		Vertices.Add(Point.Position + FVector(-HalfSize.X, HalfSize.Y, HalfSize.Z));
		
		// Add vertex colors
		for (int32 j = 0; j < 8; j++)
		{
			VertexColors.Add(Point.Color);
			Normals.Add(FVector::UpVector);
			UV0.Add(FVector2D(0, 0));
		}
		
		// Add triangles for cube faces
		// Front face
		Triangles.Append({BaseVertex + 0, BaseVertex + 1, BaseVertex + 2});
		Triangles.Append({BaseVertex + 0, BaseVertex + 2, BaseVertex + 3});
		// Back face
		Triangles.Append({BaseVertex + 5, BaseVertex + 4, BaseVertex + 7});
		Triangles.Append({BaseVertex + 5, BaseVertex + 7, BaseVertex + 6});
		// Left face
		Triangles.Append({BaseVertex + 4, BaseVertex + 0, BaseVertex + 3});
		Triangles.Append({BaseVertex + 4, BaseVertex + 3, BaseVertex + 7});
		// Right face
		Triangles.Append({BaseVertex + 1, BaseVertex + 5, BaseVertex + 6});
		Triangles.Append({BaseVertex + 1, BaseVertex + 6, BaseVertex + 2});
		// Top face
		Triangles.Append({BaseVertex + 3, BaseVertex + 2, BaseVertex + 6});
		Triangles.Append({BaseVertex + 3, BaseVertex + 6, BaseVertex + 7});
		// Bottom face
		Triangles.Append({BaseVertex + 4, BaseVertex + 5, BaseVertex + 1});
		Triangles.Append({BaseVertex + 4, BaseVertex + 1, BaseVertex + 0});
	}

	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, true);
}

void UProceduralGenerator::GenerateMesh(const TArray<FBitmapPoint>& Points)
{
	CreateProceduralMeshIfNeeded();
	ProceduralMesh->ClearAllMeshSections();

	if (Points.Num() < 3)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// Simple triangulation - connect sequential points
	for (const FBitmapPoint& Point : Points)
	{
		Vertices.Add(Point.Position);
		Normals.Add(Point.Normal);
		VertexColors.Add(Point.Color);
		UV0.Add(FVector2D(0, 0));
	}

	// Create triangles using a simple fan triangulation
	for (int32 i = 1; i < Points.Num() - 1; i++)
	{
		Triangles.Add(0);
		Triangles.Add(i);
		Triangles.Add(i + 1);
	}

	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, true);
}

void UProceduralGenerator::GenerateVoxels(const TArray<FBitmapPoint>& Points)
{
	// Voxel generation - creates a voxel grid
	CreateProceduralMeshIfNeeded();
	ProceduralMesh->ClearAllMeshSections();

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// Snap points to voxel grid and create cubes
	TSet<FIntVector> VoxelSet;
	
	for (const FBitmapPoint& Point : Points)
	{
		FIntVector VoxelPos(
			FMath::FloorToInt(Point.Position.X / VoxelSize),
			FMath::FloorToInt(Point.Position.Y / VoxelSize),
			FMath::FloorToInt(Point.Position.Z / VoxelSize)
		);
		VoxelSet.Add(VoxelPos);
	}

	// Generate mesh for each voxel
	for (const FIntVector& VoxelPos : VoxelSet)
	{
		FVector WorldPos(VoxelPos.X * VoxelSize, VoxelPos.Y * VoxelSize, VoxelPos.Z * VoxelSize);
		int32 BaseVertex = Vertices.Num();
		
		FVector HalfSize(VoxelSize * 0.5f);
		
		// Add 8 vertices for voxel cube
		Vertices.Add(WorldPos + FVector(-HalfSize.X, -HalfSize.Y, -HalfSize.Z));
		Vertices.Add(WorldPos + FVector(HalfSize.X, -HalfSize.Y, -HalfSize.Z));
		Vertices.Add(WorldPos + FVector(HalfSize.X, HalfSize.Y, -HalfSize.Z));
		Vertices.Add(WorldPos + FVector(-HalfSize.X, HalfSize.Y, -HalfSize.Z));
		Vertices.Add(WorldPos + FVector(-HalfSize.X, -HalfSize.Y, HalfSize.Z));
		Vertices.Add(WorldPos + FVector(HalfSize.X, -HalfSize.Y, HalfSize.Z));
		Vertices.Add(WorldPos + FVector(HalfSize.X, HalfSize.Y, HalfSize.Z));
		Vertices.Add(WorldPos + FVector(-HalfSize.X, HalfSize.Y, HalfSize.Z));
		
		for (int32 j = 0; j < 8; j++)
		{
			VertexColors.Add(FColor::White);
			Normals.Add(FVector::UpVector);
			UV0.Add(FVector2D(0, 0));
		}
		
		// Add cube faces
		Triangles.Append({BaseVertex + 0, BaseVertex + 1, BaseVertex + 2});
		Triangles.Append({BaseVertex + 0, BaseVertex + 2, BaseVertex + 3});
		Triangles.Append({BaseVertex + 5, BaseVertex + 4, BaseVertex + 7});
		Triangles.Append({BaseVertex + 5, BaseVertex + 7, BaseVertex + 6});
		Triangles.Append({BaseVertex + 4, BaseVertex + 0, BaseVertex + 3});
		Triangles.Append({BaseVertex + 4, BaseVertex + 3, BaseVertex + 7});
		Triangles.Append({BaseVertex + 1, BaseVertex + 5, BaseVertex + 6});
		Triangles.Append({BaseVertex + 1, BaseVertex + 6, BaseVertex + 2});
		Triangles.Append({BaseVertex + 3, BaseVertex + 2, BaseVertex + 6});
		Triangles.Append({BaseVertex + 3, BaseVertex + 6, BaseVertex + 7});
		Triangles.Append({BaseVertex + 4, BaseVertex + 5, BaseVertex + 1});
		Triangles.Append({BaseVertex + 4, BaseVertex + 1, BaseVertex + 0});
	}

	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, true);
}

void UProceduralGenerator::GenerateSurface(const TArray<FBitmapPoint>& Points)
{
	// Surface generation - creates a continuous surface from points
	GenerateMesh(Points);
}

int32 UProceduralGenerator::GetCachedPointsMemoryKB() const
{
	const int32 PointSize = sizeof(FBitmapPoint);
	const int32 ArrayOverhead = sizeof(TArray<FBitmapPoint>);
	const int32 TotalBytes = (CachedPoints.Num() * PointSize) + ArrayOverhead;
	return TotalBytes / 1024;
}

void UProceduralGenerator::ForceMemoryCleanup()
{
	// Clear all mesh sections
	if (ProceduralMesh)
	{
		ProceduralMesh->ClearAllMeshSections();
	}
	
	// Clear and shrink cached points array
	const int32 PreviousMemory = GetCachedPointsMemoryKB();
	CachedPoints.Empty();
	CachedPoints.Shrink();
	
	UE_LOG(LogTemp, Log, TEXT("ProceduralGenerator: Force cleanup freed %d KB of cached point data"), PreviousMemory);
}

void UProceduralGenerator::SetMarchingCubesConfig(const FMarchingCubesConfig& NewConfig)
{
	MarchingCubesConfig = NewConfig;
	
	// Update voxel size to match config
	VoxelSize = MarchingCubesConfig.VoxelSize;
	
	UE_LOG(LogTemp, Log, TEXT("Marching Cubes config updated: VoxelSize=%.2f, GridRes=(%d,%d,%d), IsoValue=%.2f"), 
		MarchingCubesConfig.VoxelSize, 
		MarchingCubesConfig.GridResolution.X, 
		MarchingCubesConfig.GridResolution.Y, 
		MarchingCubesConfig.GridResolution.Z,
		MarchingCubesConfig.IsoValue);
}

void UProceduralGenerator::GenerateMarchingCubes(const TArray<FBitmapPoint>& Points)
{
	if (!MarchingCubesGenerator)
	{
		UE_LOG(LogTemp, Error, TEXT("Marching cubes generator not initialized"));
		return;
	}
	
	// Update grid bounds from points if needed
	UpdateGridBoundsFromPoints(Points);
	
	// Generate using marching cubes
	GenerateMarchingCubesInternal(Points);
}

void UProceduralGenerator::UpdateGridBoundsFromPoints(const TArray<FBitmapPoint>& Points, float Padding)
{
	if (Points.Num() == 0)
	{
		return;
	}
	
	// Find bounding box of all points
	FVector MinBounds = Points[0].Position;
	FVector MaxBounds = Points[0].Position;
	
	for (const FBitmapPoint& Point : Points)
	{
		MinBounds = MinBounds.ComponentMin(Point.Position);
		MaxBounds = MaxBounds.ComponentMax(Point.Position);
	}
	
	// Add padding
	MinBounds -= FVector(Padding);
	MaxBounds += FVector(Padding);
	
	// Update marching cubes config
	MarchingCubesConfig.GridMin = MinBounds;
	MarchingCubesConfig.GridMax = MaxBounds;
	
	// Calculate optimal grid resolution based on voxel size
	FVector GridSize = MaxBounds - MinBounds;
	FIntVector OptimalResolution = FIntVector(
		FMath::CeilToInt(GridSize.X / MarchingCubesConfig.VoxelSize),
		FMath::CeilToInt(GridSize.Y / MarchingCubesConfig.VoxelSize),
		FMath::CeilToInt(GridSize.Z / MarchingCubesConfig.VoxelSize)
	);
	
	// Clamp resolution to reasonable limits
	OptimalResolution.X = FMath::Clamp(OptimalResolution.X, 10, 200);
	OptimalResolution.Y = FMath::Clamp(OptimalResolution.Y, 10, 200);
	OptimalResolution.Z = FMath::Clamp(OptimalResolution.Z, 10, 200);
	
	MarchingCubesConfig.GridResolution = OptimalResolution;
	
	UE_LOG(LogTemp, Log, TEXT("Updated grid bounds: Min=(%s), Max=(%s), Resolution=(%d,%d,%d)"), 
		*MinBounds.ToString(), *MaxBounds.ToString(),
		OptimalResolution.X, OptimalResolution.Y, OptimalResolution.Z);
}

void UProceduralGenerator::GenerateMarchingCubesInternal(const TArray<FBitmapPoint>& Points)
{
	if (!MarchingCubesGenerator || Points.Num() == 0)
	{
		return;
	}
	
	CreateProceduralMeshIfNeeded();
	
	// Clear existing mesh
	ProceduralMesh->ClearAllMeshSections();
	
	// Generate triangles using marching cubes
	TArray<FMCTriangle> MCTriangles = MarchingCubesGenerator->GenerateFromBitmapPoints(Points, MarchingCubesConfig);
	
	if (MCTriangles.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Marching cubes generated no triangles"));
		return;
	}
	
	// Convert marching cubes triangles to procedural mesh format
	ConvertMCTrianglesToMesh(MCTriangles);
	
	UE_LOG(LogTemp, Log, TEXT("Marching cubes generated %d triangles from %d points"), MCTriangles.Num(), Points.Num());
}

void UProceduralGenerator::ConvertMCTrianglesToMesh(const TArray<FMCTriangle>& MCTriangles)
{
	if (!ProceduralMesh || MCTriangles.Num() == 0)
	{
		return;
	}
	
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	
	// Convert marching cubes triangles to mesh data
	for (int32 i = 0; i < MCTriangles.Num(); i++)
	{
		const FMCTriangle& Triangle = MCTriangles[i];
		
		// Add vertices
		int32 BaseIndex = Vertices.Num();
		for (int32 j = 0; j < 3; j++)
		{
			Vertices.Add(Triangle.Vertices[j]);
			Normals.Add(Triangle.Normals[j]);
			UV0.Add(Triangle.UVs[j]);
			VertexColors.Add(Triangle.Colors[j]);
		}
		
		// Add triangle indices
		Triangles.Add(BaseIndex);
		Triangles.Add(BaseIndex + 1);
		Triangles.Add(BaseIndex + 2);
	}
	
	// Calculate tangents
	for (int32 i = 0; i < Vertices.Num(); i++)
	{
		FVector Tangent = FVector::ForwardVector;
		if (i < Normals.Num())
		{
			// Calculate tangent perpendicular to normal
			FVector Normal = Normals[i];
			if (!Normal.Equals(FVector::ForwardVector))
			{
				Tangent = FVector::CrossProduct(Normal, FVector::UpVector).GetSafeNormal();
			}
	}
	Tangents.Add(FProcMeshTangent(Tangent, false));
}

// Create mesh section
ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, true);

// Apply material if set
if (DefaultMaterial)
{
	ProceduralMesh->SetMaterial(0, DefaultMaterial);
}

UE_LOG(LogTemp, Log, TEXT("Created mesh with %d vertices and %d triangles"), Vertices.Num(), Triangles.Num() / 3);
}

// Async Generation Methods

int32 UProceduralGenerator::GenerateAsyncFromBitmapPoints(const TArray<FBitmapPoint>& Points, bool bForceAsync)
{
	if (!bEnableAsyncGeneration || !MeshGenerationManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProceduralGenerator: Async generation not available"));
		return -1;
	}

	// Check if we should use async generation
	if (!bForceAsync && !ShouldUseAsyncGeneration(Points.Num()))
	{
		return -1;
	}

	EMeshGenerationTaskType TaskType = GetTaskTypeFromGenerationType();
	
	// Submit job to mesh generation manager
	FOnMeshGenerationComplete CompletionCallback;
	CompletionCallback.BindUObject(this, &UProceduralGenerator::ApplyAsyncResult);
	
	int32 JobID = MeshGenerationManager->SubmitMeshGenerationJob(
		Points,
		TaskType,
		MarchingCubesConfig,
		VoxelSize,
		CompletionCallback
	);

	if (JobID != -1)
	{
		// Add to active jobs list
		FScopeLock Lock(&AsyncJobsMutex);
		ActiveAsyncJobs.Add(JobID);
		
		UE_LOG(LogTemp, Log, TEXT("ProceduralGenerator: Started async job %d for %d points"), JobID, Points.Num());
	}

	return JobID;
}

bool UProceduralGenerator::CancelAsyncGeneration(int32 JobID)
{
	if (!MeshGenerationManager)
	{
		return false;
	}

	bool bCancelled = MeshGenerationManager->CancelJob(JobID);
	
	if (bCancelled)
	{
		FScopeLock Lock(&AsyncJobsMutex);
		ActiveAsyncJobs.Remove(JobID);
		UE_LOG(LogTemp, Log, TEXT("ProceduralGenerator: Cancelled async job %d"), JobID);
	}

	return bCancelled;
}

float UProceduralGenerator::GetAsyncGenerationProgress(int32 JobID) const
{
	if (!MeshGenerationManager)
	{
		return 0.0f;
	}

	FMeshGenerationJobInfo JobInfo;
	if (MeshGenerationManager->GetJobInfo(JobID, JobInfo))
	{
		return JobInfo.Progress;
	}

	return 0.0f;
}

bool UProceduralGenerator::IsAsyncGenerationActive() const
{
	FScopeLock Lock(&AsyncJobsMutex);
	return ActiveAsyncJobs.Num() > 0;
}

void UProceduralGenerator::SetAsyncThreshold(int32 NewThreshold)
{
	AsyncGenerationThreshold = FMath::Max(1000, NewThreshold);
	UE_LOG(LogTemp, Log, TEXT("ProceduralGenerator: Async threshold set to %d points"), AsyncGenerationThreshold);
}

bool UProceduralGenerator::ShouldUseAsyncGeneration(int32 PointCount) const
{
	return bEnableAsyncGeneration && 
		   MeshGenerationManager && 
		   PointCount >= AsyncGenerationThreshold;
}

EMeshGenerationTaskType UProceduralGenerator::GetTaskTypeFromGenerationType() const
{
	switch (GenerationType)
	{
	case EProceduralGenerationType::PointCloud:
		return EMeshGenerationTaskType::PointCloud;
	case EProceduralGenerationType::Mesh:
		return EMeshGenerationTaskType::Mesh;
	case EProceduralGenerationType::Voxel:
		return EMeshGenerationTaskType::Voxel;
	case EProceduralGenerationType::MarchingCubes:
		return EMeshGenerationTaskType::MarchingCubes;
	case EProceduralGenerationType::Surface:
	default:
		return EMeshGenerationTaskType::Mesh;
	}
}

void UProceduralGenerator::OnAsyncJobCompleted(int32 JobID, bool bSuccess)
{
	UE_LOG(LogTemp, Log, TEXT("ProceduralGenerator: Async job %d completed %s"), 
		JobID, bSuccess ? TEXT("successfully") : TEXT("with failure"));
	
	// Remove from active jobs
	{
		FScopeLock Lock(&AsyncJobsMutex);
		ActiveAsyncJobs.Remove(JobID);
	}
	
	// Broadcast completion event
	OnAsyncGenerationComplete.Broadcast(bSuccess, JobID);
}

void UProceduralGenerator::ApplyAsyncResult(int32 JobID, const FMeshGenerationResult& Result)
{
	if (!MeshGenerationManager)
	{
		return;
	}

	// Get the result from the manager
	FMeshGenerationResult JobResult;
	if (!MeshGenerationManager->GetJobResult(JobID, JobResult))
	{
		UE_LOG(LogTemp, Warning, TEXT("ProceduralGenerator: Failed to get result for job %d"), JobID);
		OnAsyncJobCompleted(JobID, false);
		return;
	}

	// Apply result to procedural mesh on game thread
	AsyncTask(ENamedThreads::GameThread, [this, JobResult, JobID]()
	{
		CreateProceduralMeshIfNeeded();
		
		// Clear existing mesh
		ProceduralMesh->ClearAllMeshSections();
		
		// Apply the generated mesh data
		ProceduralMesh->CreateMeshSection(0, 
			JobResult.Vertices, 
			JobResult.Triangles, 
			JobResult.Normals, 
			JobResult.UV0, 
			JobResult.VertexColors, 
			JobResult.Tangents, 
			true);
		
		// Apply material if set
		if (DefaultMaterial)
		{
			ProceduralMesh->SetMaterial(0, DefaultMaterial);
		}
		
		UE_LOG(LogTemp, Log, TEXT("ProceduralGenerator: Applied async result - %d vertices, %d triangles, %.3fs execution time"), 
			JobResult.Vertices.Num(), JobResult.TriangleCount, JobResult.ExecutionTime);
		
		OnAsyncJobCompleted(JobID, true);
	});
}

void UProceduralGenerator::CleanupCompletedAsyncJobs()
{
	if (!MeshGenerationManager)
	{
		return;
	}

	FScopeLock Lock(&AsyncJobsMutex);
	
	// Check for completed jobs and remove them from active list
	TArray<int32> JobsToRemove;
	
	for (int32 JobID : ActiveAsyncJobs)
	{
		FMeshGenerationJobInfo JobInfo;
		if (MeshGenerationManager->GetJobInfo(JobID, JobInfo))
		{
			if (JobInfo.Status == EMeshGenerationTaskStatus::Completed ||
				JobInfo.Status == EMeshGenerationTaskStatus::Failed ||
				JobInfo.Status == EMeshGenerationTaskStatus::Cancelled)
			{
				JobsToRemove.Add(JobID);
			}
		}
		else
		{
			// Job doesn't exist anymore, remove it
			JobsToRemove.Add(JobID);
		}
	}
	
	for (int32 JobID : JobsToRemove)
	{
		ActiveAsyncJobs.Remove(JobID);
	}
}

void UProceduralGenerator::HandleTrackingLoss(ETrackingState PreviousState, const FString& LossReason)
{
	FScopeLock Lock(&TrackingStateMutex);
	
	if (bIsTrackingLost)
	{
		return; // Already handling tracking loss
	}
	
	bIsTrackingLost = true;
	TrackingLossStartTime = FPlatformTime::Seconds();
	CurrentTrackingState = ETrackingState::TrackingLost;
	
	UE_LOG(LogTemp, Warning, TEXT("AR Tracking Lost: %s (Previous State: %d)"), *LossReason, (int32)PreviousState);
	
	// Store current geometry snapshot for potential recovery
	if (bUseSpatialAnchors && !CachedPoints.IsEmpty())
	{
		PreLossGeometrySnapshot = CachedPoints;
		StoreSpatialAnchor(GetOwner()->GetActorTransform(), FString::Printf(TEXT("PreLoss_%d"), FMath::RandRange(1000, 9999)));
	}
	
	// Freeze mesh if configured
	if (bFreezeMeshOnTrackingLoss && ProceduralMesh)
	{
		ProceduralMesh->SetVisibility(false);
	}
	
	// Cancel active async jobs to preserve resources
	if (MeshGenerationManager)
	{
		for (int32 JobID : ActiveAsyncJobs)
		{
			MeshGenerationManager->CancelJob(JobID);
		}
		ActiveAsyncJobs.Empty();
	}
	
	// Broadcast tracking loss event
	OnTrackingLoss.Broadcast(PreviousState);
}

void UProceduralGenerator::HandleTrackingRecovery(ETrackingState NewState, float LostDuration)
{
	FScopeLock Lock(&TrackingStateMutex);
	
	if (!bIsTrackingLost)
	{
		return; // Not in tracking loss state
	}
	
	bIsTrackingLost = false;
	CurrentTrackingState = NewState;
	
	UE_LOG(LogTemp, Log, TEXT("AR Tracking Recovered: New State %d after %.2f seconds"), (int32)NewState, LostDuration);
	
	// Restore mesh visibility
	if (bFreezeMeshOnTrackingLoss && ProceduralMesh)
	{
		ProceduralMesh->SetVisibility(true);
	}
	
	// Auto-recover geometry if enabled
	if (bAutoRecoverFromTrackingLoss && !PreLossGeometrySnapshot.IsEmpty())
	{
		// Restore from spatial anchor if available
		if (bUseSpatialAnchors && !CurrentAnchorID.IsEmpty())
		{
			if (!RestoreFromSpatialAnchor(CurrentAnchorID))
			{
				// Fallback to geometry snapshot
				GenerateFromBitmapPoints(PreLossGeometrySnapshot);
			}
		}
		else
		{
			// Direct geometry restoration
			GenerateFromBitmapPoints(PreLossGeometrySnapshot);
		}
	}
	
	// Broadcast tracking recovery event
	OnTrackingRecovery.Broadcast(NewState, LostDuration);
}

void UProceduralGenerator::StoreSpatialAnchor(const FTransform& AnchorTransform, const FString& AnchorID)
{
	FScopeLock Lock(&TrackingStateMutex);
	
	LastKnownAnchorTransform = AnchorTransform;
	CurrentAnchorID = AnchorID.IsEmpty() ? FString::Printf(TEXT("Anchor_%d"), FMath::RandRange(1000, 9999)) : AnchorID;
	
	UE_LOG(LogTemp, Log, TEXT("Spatial anchor stored: %s at %s"), *CurrentAnchorID, *AnchorTransform.ToString());
	
	// In a real implementation, this would interface with the AR platform's spatial anchor system
	// For now, we just store the transform locally
}

bool UProceduralGenerator::RestoreFromSpatialAnchor(const FString& AnchorID)
{
	FScopeLock Lock(&TrackingStateMutex);
	
	if (AnchorID != CurrentAnchorID || LastKnownAnchorTransform.Equals(FTransform::Identity))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to restore from spatial anchor: %s"), *AnchorID);
		return false;
	}
	
	// Restore actor transform
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorTransform(LastKnownAnchorTransform);
	}
	
	// Restore geometry if available
	if (!PreLossGeometrySnapshot.IsEmpty())
	{
		GenerateFromBitmapPoints(PreLossGeometrySnapshot);
	}
	
	UE_LOG(LogTemp, Log, TEXT("Successfully restored from spatial anchor: %s"), *AnchorID);
	return true;
}

void UProceduralGenerator::UpdateTrackingQuality(float NewQuality)
{
	FScopeLock Lock(&TrackingStateMutex);
	
	float OldQuality = CurrentTrackingQuality;
	CurrentTrackingQuality = FMath::Clamp(NewQuality, 0.0f, 1.0f);
	
	// Check for significant quality changes
	if (FMath::Abs(OldQuality - CurrentTrackingQuality) > 0.1f)
	{
		OnTrackingQualityChange.Broadcast(OldQuality, CurrentTrackingQuality);
		
		UE_LOG(LogTemp, Log, TEXT("Tracking quality changed: %.2f -> %.2f"), OldQuality, CurrentTrackingQuality);
		
		// Handle low quality threshold
		if (CurrentTrackingQuality < TrackingQualityThreshold && !bIsTrackingLost)
		{
			UE_LOG(LogTemp, Warning, TEXT("Tracking quality below threshold: %.2f < %.2f"), CurrentTrackingQuality, TrackingQualityThreshold);
			
			// Reduce generation frequency to save resources
			if (bAutoUpdate)
			{
				UpdateInterval = FMath::Max(UpdateInterval * 2.0f, 0.5f);
			}
		}
		else if (CurrentTrackingQuality >= TrackingQualityThreshold && OldQuality < TrackingQualityThreshold)
		{
			// Restore normal update frequency
			UpdateInterval = 0.1f;
		}
	}
}