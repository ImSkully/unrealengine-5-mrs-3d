#include "ProceduralGenerator.h"
#include "MRBitmapMapper.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"

UProceduralGenerator::UProceduralGenerator()
	: GenerationType(EProceduralGenerationType::Mesh)
	, VoxelSize(10.0f)
	, bAutoUpdate(true)
	, UpdateInterval(0.1f)
	, TimeSinceLastUpdate(0.0f)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UProceduralGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	CreateProceduralMeshIfNeeded();
	
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
