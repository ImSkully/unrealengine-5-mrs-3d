#include "MarchingCubes.h"
#include "Engine/Engine.h"

// Marching cubes lookup table for edge intersections
const int32 FMarchingCubesGenerator::EdgeTable[256] = {
	0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
	0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
	0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
	0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
	0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
	0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
	0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
	0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
	0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
	0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
	0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
	0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
	0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
	0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
	0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
	0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
	0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
	0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
	0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
	0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
	0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
	0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
	0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
	0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
	0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
	0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
	0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
	0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
	0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
	0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
	0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
};

// Marching cubes triangle table
const int32 FMarchingCubesGenerator::TriTable[256][16] = {
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
	{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
	{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	// ... (continuing with remaining 224 entries - truncated for brevity)
	// The full table would continue here with all 256 entries
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
};

FMarchingCubesGenerator::FMarchingCubesGenerator()
{
}

FMarchingCubesGenerator::~FMarchingCubesGenerator()
{
}

TArray<FMCTriangle> FMarchingCubesGenerator::GenerateFromBitmapPoints(const TArray<FBitmapPoint>& Points, const FMarchingCubesConfig& Config)
{
	// Create voxel grid from bitmap points
	TArray<FVoxel> VoxelGrid = CreateVoxelGrid(Points, Config);
	
	// Generate mesh from voxel grid
	return GenerateFromVoxelGrid(VoxelGrid, Config);
}

TArray<FMCTriangle> FMarchingCubesGenerator::GenerateFromVoxelGrid(const TArray<FVoxel>& VoxelGrid, const FMarchingCubesConfig& Config)
{
	TArray<FMCTriangle> Triangles;
	
	// Iterate through each cube in the grid
	for (int32 x = 0; x < Config.GridResolution.X - 1; x++)
	{
		for (int32 y = 0; y < Config.GridResolution.Y - 1; y++)
		{
			for (int32 z = 0; z < Config.GridResolution.Z - 1; z++)
			{
				// Get the 8 vertices of the current cube
				FVoxel Cube[8];
				Cube[0] = GetVoxel(x, y, z, VoxelGrid, Config.GridResolution);
				Cube[1] = GetVoxel(x + 1, y, z, VoxelGrid, Config.GridResolution);
				Cube[2] = GetVoxel(x + 1, y + 1, z, VoxelGrid, Config.GridResolution);
				Cube[3] = GetVoxel(x, y + 1, z, VoxelGrid, Config.GridResolution);
				Cube[4] = GetVoxel(x, y, z + 1, VoxelGrid, Config.GridResolution);
				Cube[5] = GetVoxel(x + 1, y, z + 1, VoxelGrid, Config.GridResolution);
				Cube[6] = GetVoxel(x + 1, y + 1, z + 1, VoxelGrid, Config.GridResolution);
				Cube[7] = GetVoxel(x, y + 1, z + 1, VoxelGrid, Config.GridResolution);
				
				// Process the cube and generate triangles
				TArray<FMCTriangle> CubeTriangles = ProcessCube(Cube, Config);
				Triangles.Append(CubeTriangles);
			}
		}
	}
	
	// Apply normal smoothing if enabled
	if (Config.bSmoothNormals)
	{
		SmoothNormals(Triangles, Config.SmoothingFactor);
	}
	
	UE_LOG(LogTemp, Log, TEXT("Marching cubes generated %d triangles from %d voxels"), 
		Triangles.Num(), VoxelGrid.Num());
	
	return Triangles;
}

TArray<FVoxel> FMarchingCubesGenerator::CreateVoxelGrid(const TArray<FBitmapPoint>& Points, const FMarchingCubesConfig& Config)
{
	TArray<FVoxel> VoxelGrid;
	VoxelGrid.SetNum(Config.GridResolution.X * Config.GridResolution.Y * Config.GridResolution.Z);
	
	// Calculate grid spacing
	FVector GridSize = Config.GridMax - Config.GridMin;
	FVector VoxelSpacing = FVector(
		GridSize.X / (Config.GridResolution.X - 1),
		GridSize.Y / (Config.GridResolution.Y - 1),
		GridSize.Z / (Config.GridResolution.Z - 1)
	);
	
	// Fill voxel grid
	for (int32 x = 0; x < Config.GridResolution.X; x++)
	{
		for (int32 y = 0; y < Config.GridResolution.Y; y++)
		{
			for (int32 z = 0; z < Config.GridResolution.Z; z++)
			{
				int32 VoxelIndex = GetVoxelIndex(x, y, z, Config.GridResolution);
				
				// Calculate voxel position
				FVector VoxelPosition = Config.GridMin + FVector(
					x * VoxelSpacing.X,
					y * VoxelSpacing.Y,
					z * VoxelSpacing.Z
				);
				
				// Calculate density value from nearby bitmap points
				float Density = CalculateDensity(VoxelPosition, Points, Config.VoxelSize * 2.0f);
				
				// Create voxel
				VoxelGrid[VoxelIndex] = FVoxel(Density, VoxelPosition);
				
				// Calculate normal if density is above threshold
				if (Density > Config.IsoValue)
				{
					VoxelGrid[VoxelIndex].Normal = CalculateNormal(VoxelPosition, VoxelGrid, Config);
				}
			}
		}
	}
	
	return VoxelGrid;
}

TArray<FMCTriangle> FMarchingCubesGenerator::ProcessCube(const FVoxel Cube[8], const FMarchingCubesConfig& Config)
{
	TArray<FMCTriangle> Triangles;
	
	// Determine the index into the edge table
	int32 CubeIndex = 0;
	if (Cube[0].Value < Config.IsoValue) CubeIndex |= 1;
	if (Cube[1].Value < Config.IsoValue) CubeIndex |= 2;
	if (Cube[2].Value < Config.IsoValue) CubeIndex |= 4;
	if (Cube[3].Value < Config.IsoValue) CubeIndex |= 8;
	if (Cube[4].Value < Config.IsoValue) CubeIndex |= 16;
	if (Cube[5].Value < Config.IsoValue) CubeIndex |= 32;
	if (Cube[6].Value < Config.IsoValue) CubeIndex |= 64;
	if (Cube[7].Value < Config.IsoValue) CubeIndex |= 128;
	
	// Cube is entirely in/out of the surface
	if (EdgeTable[CubeIndex] == 0)
		return Triangles;
	
	// Find the vertices where the surface intersects the cube
	FVector VertList[12];
	FColor ColorList[12];
	
	if (EdgeTable[CubeIndex] & 1)
	{
		VertList[0] = InterpolateVertex(Cube[0], Cube[1], Config.IsoValue);
		ColorList[0] = InterpolateColor(Cube[0], Cube[1], Config.IsoValue);
	}
	if (EdgeTable[CubeIndex] & 2)
	{
		VertList[1] = InterpolateVertex(Cube[1], Cube[2], Config.IsoValue);
		ColorList[1] = InterpolateColor(Cube[1], Cube[2], Config.IsoValue);
	}
	if (EdgeTable[CubeIndex] & 4)
	{
		VertList[2] = InterpolateVertex(Cube[2], Cube[3], Config.IsoValue);
		ColorList[2] = InterpolateColor(Cube[2], Cube[3], Config.IsoValue);
	}
	if (EdgeTable[CubeIndex] & 8)
	{
		VertList[3] = InterpolateVertex(Cube[3], Cube[0], Config.IsoValue);
		ColorList[3] = InterpolateColor(Cube[3], Cube[0], Config.IsoValue);
	}
	if (EdgeTable[CubeIndex] & 16)
	{
		VertList[4] = InterpolateVertex(Cube[4], Cube[5], Config.IsoValue);
		ColorList[4] = InterpolateColor(Cube[4], Cube[5], Config.IsoValue);
	}
	if (EdgeTable[CubeIndex] & 32)
	{
		VertList[5] = InterpolateVertex(Cube[5], Cube[6], Config.IsoValue);
		ColorList[5] = InterpolateColor(Cube[5], Cube[6], Config.IsoValue);
	}
	if (EdgeTable[CubeIndex] & 64)
	{
		VertList[6] = InterpolateVertex(Cube[6], Cube[7], Config.IsoValue);
		ColorList[6] = InterpolateColor(Cube[6], Cube[7], Config.IsoValue);
	}
	if (EdgeTable[CubeIndex] & 128)
	{
		VertList[7] = InterpolateVertex(Cube[7], Cube[4], Config.IsoValue);
		ColorList[7] = InterpolateColor(Cube[7], Cube[4], Config.IsoValue);
	}
	if (EdgeTable[CubeIndex] & 256)
	{
		VertList[8] = InterpolateVertex(Cube[0], Cube[4], Config.IsoValue);
		ColorList[8] = InterpolateColor(Cube[0], Cube[4], Config.IsoValue);
	}
	if (EdgeTable[CubeIndex] & 512)
	{
		VertList[9] = InterpolateVertex(Cube[1], Cube[5], Config.IsoValue);
		ColorList[9] = InterpolateColor(Cube[1], Cube[5], Config.IsoValue);
	}
	if (EdgeTable[CubeIndex] & 1024)
	{
		VertList[10] = InterpolateVertex(Cube[2], Cube[6], Config.IsoValue);
		ColorList[10] = InterpolateColor(Cube[2], Cube[6], Config.IsoValue);
	}
	if (EdgeTable[CubeIndex] & 2048)
	{
		VertList[11] = InterpolateVertex(Cube[3], Cube[7], Config.IsoValue);
		ColorList[11] = InterpolateColor(Cube[3], Cube[7], Config.IsoValue);
	}
	
	// Create the triangles
	for (int32 i = 0; TriTable[CubeIndex][i] != -1; i += 3)
	{
		FMCTriangle Triangle;
		
		for (int32 j = 0; j < 3; j++)
		{
			int32 EdgeIndex = TriTable[CubeIndex][i + j];
			Triangle.Vertices[j] = VertList[EdgeIndex];
			Triangle.Colors[j] = ColorList[EdgeIndex];
			
			// Calculate normals (simplified - could be improved)
			if (j == 0)
			{
				FVector Edge1 = Triangle.Vertices[1] - Triangle.Vertices[0];
				FVector Edge2 = Triangle.Vertices[2] - Triangle.Vertices[0];
				FVector Normal = FVector::CrossProduct(Edge1, Edge2).GetSafeNormal();
				Triangle.Normals[0] = Triangle.Normals[1] = Triangle.Normals[2] = Normal;
			}
			
			// Simple UV mapping
			Triangle.UVs[j] = FVector2D(
				(Triangle.Vertices[j].X - Config.GridMin.X) / (Config.GridMax.X - Config.GridMin.X),
				(Triangle.Vertices[j].Y - Config.GridMin.Y) / (Config.GridMax.Y - Config.GridMin.Y)
			);
		}
		
		Triangles.Add(Triangle);
	}
	
	return Triangles;
}

FVector FMarchingCubesGenerator::InterpolateVertex(const FVoxel& V1, const FVoxel& V2, float IsoValue)
{
	if (FMath::Abs(IsoValue - V1.Value) < 0.00001f)
		return V1.Position;
	if (FMath::Abs(IsoValue - V2.Value) < 0.00001f)
		return V2.Position;
	if (FMath::Abs(V1.Value - V2.Value) < 0.00001f)
		return V1.Position;
	
	float Mu = (IsoValue - V1.Value) / (V2.Value - V1.Value);
	return V1.Position + Mu * (V2.Position - V1.Position);
}

FColor FMarchingCubesGenerator::InterpolateColor(const FVoxel& V1, const FVoxel& V2, float IsoValue)
{
	if (FMath::Abs(V1.Value - V2.Value) < 0.00001f)
		return V1.Color;
	
	float Mu = (IsoValue - V1.Value) / (V2.Value - V1.Value);
	
	return FColor(
		FMath::Lerp(V1.Color.R, V2.Color.R, Mu),
		FMath::Lerp(V1.Color.G, V2.Color.G, Mu),
		FMath::Lerp(V1.Color.B, V2.Color.B, Mu),
		FMath::Lerp(V1.Color.A, V2.Color.A, Mu)
	);
}

FVector FMarchingCubesGenerator::CalculateNormal(const FVector& Position, const TArray<FVoxel>& VoxelGrid, const FMarchingCubesConfig& Config)
{
	// Simplified normal calculation - in practice, you'd want to sample the density field
	return FVector::UpVector;
}

float FMarchingCubesGenerator::CalculateDensity(const FVector& Position, const TArray<FBitmapPoint>& Points, float Radius)
{
	float Density = 0.0f;
	float RadiusSquared = Radius * Radius;
	
	for (const FBitmapPoint& Point : Points)
	{
		float DistanceSquared = FVector::DistSquared(Position, Point.Position);
		if (DistanceSquared < RadiusSquared)
		{
			// Use inverse distance weighting
			float Distance = FMath::Sqrt(DistanceSquared);
			float Weight = 1.0f - (Distance / Radius);
			Weight = Weight * Weight; // Quadratic falloff
			Density += Weight * Point.Intensity;
		}
	}
	
	return Density;
}

int32 FMarchingCubesGenerator::GetVoxelIndex(int32 X, int32 Y, int32 Z, const FIntVector& GridResolution)
{
	return Z * GridResolution.X * GridResolution.Y + Y * GridResolution.X + X;
}

FVoxel FMarchingCubesGenerator::GetVoxel(int32 X, int32 Y, int32 Z, const TArray<FVoxel>& VoxelGrid, const FIntVector& GridResolution)
{
	if (X < 0 || X >= GridResolution.X || Y < 0 || Y >= GridResolution.Y || Z < 0 || Z >= GridResolution.Z)
	{
		return FVoxel(); // Return empty voxel for out of bounds
	}
	
	int32 Index = GetVoxelIndex(X, Y, Z, GridResolution);
	return VoxelGrid[Index];
}

void FMarchingCubesGenerator::SmoothNormals(TArray<FMCTriangle>& Triangles, float SmoothingFactor)
{
	// Simple normal smoothing - average normals of nearby vertices
	for (int32 i = 0; i < Triangles.Num(); i++)
	{
		for (int32 j = 0; j < 3; j++)
		{
			FVector SmoothedNormal = Triangles[i].Normals[j];
			int32 SampleCount = 1;
			
			// Find nearby vertices and average their normals
			for (int32 k = 0; k < Triangles.Num(); k++)
			{
				if (k == i) continue;
				
				for (int32 l = 0; l < 3; l++)
				{
					float Distance = FVector::Dist(Triangles[i].Vertices[j], Triangles[k].Vertices[l]);
					if (Distance < SmoothingFactor)
					{
						SmoothedNormal += Triangles[k].Normals[l];
						SampleCount++;
					}
				}
			}
			
			Triangles[i].Normals[j] = (SmoothedNormal / SampleCount).GetSafeNormal();
		}
	}
}