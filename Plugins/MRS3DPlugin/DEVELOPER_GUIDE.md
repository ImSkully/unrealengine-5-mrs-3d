# MRS3D Plugin - Developer Guide

## Architecture Overview

The MRS3D plugin consists of three main components that work together:

### 1. MRBitmapMapper (Subsystem)
A game instance subsystem that acts as the central data store for AR/MR bitmap points.

**Lifecycle:**
- Initialized when the game instance is created
- Persists throughout the game session
- Automatically cleaned up on shutdown

**Data Flow:**
```
AR/MR Device → ReceiveARData() → AddBitmapPoints() → OnBitmapPointsUpdated Event
```

### 2. ProceduralGenerator (Component)
An actor component that converts bitmap point data into 3D geometry.

**Generation Pipeline:**
```
Bitmap Points → Generation Algorithm → Procedural Mesh Component → Rendered Geometry
```

**Supported Algorithms:**
- **Point Cloud**: Simple 1:1 mapping of points to small cubes
- **Mesh**: Triangulation of point cloud into continuous surface
- **Voxel**: Grid-based voxelization with spatial hashing
- **Surface**: Advanced surface reconstruction (currently uses mesh algorithm)

### 3. MRS3DGameplayActor (Actor)
Blueprint-spawnable actor that ties everything together.

**Integration Points:**
- Hooks into gameplay framework
- Provides Blueprint-accessible functions
- Manages component lifecycle
- Handles debug visualization

## Data Structures

### FBitmapPoint
Core data structure representing a single AR/MR point:

```cpp
struct FBitmapPoint
{
    FVector Position;      // 3D world position
    FColor Color;          // RGB color data
    float Intensity;       // Point intensity/confidence
    float Timestamp;       // When the point was captured
    FVector Normal;        // Surface normal (if available)
};
```

## Integration Patterns

### Pattern 1: Direct Subsystem Access
```cpp
// Get the subsystem
UMRBitmapMapper* Mapper = GetGameInstance()->GetSubsystem<UMRBitmapMapper>();

// Add points directly
TArray<FBitmapPoint> Points;
// ... populate points from AR system ...
Mapper->AddBitmapPoints(Points);

// Listen for updates
Mapper->OnBitmapPointsUpdated.AddDynamic(this, &AMyActor::OnPointsUpdated);
```

### Pattern 2: Gameplay Actor Integration
```cpp
// Spawn the actor
AMRS3DGameplayActor* Actor = GetWorld()->SpawnActor<AMRS3DGameplayActor>();

// Configure generation
Actor->ProceduralGenerator->SetGenerationType(EProceduralGenerationType::Voxel);
Actor->ProceduralGenerator->VoxelSize = 25.0f;

// Send AR data
TArray<FVector> Positions;
TArray<FColor> Colors;
// ... get data from AR system ...
Actor->ReceiveARData(Positions, Colors);
```

### Pattern 3: Blueprint Integration
In Blueprint:
1. Place MRS3DGameplayActor in level
2. Set properties in Details panel
3. Call "Receive AR Data" or "Simulate AR Input"
4. Geometry generates automatically

## Performance Considerations

### Memory Management
- Maximum bitmap points configurable via INI file
- Consider implementing point culling for large datasets
- Use spatial partitioning for efficient queries

### Update Frequency
- `UpdateInterval` controls generation frequency
- Lower values = more responsive, higher CPU usage
- Recommended: 0.1 - 0.5 seconds for real-time AR

### Mesh Complexity
- Point cloud mode: O(n) complexity, fastest
- Voxel mode: O(n) with spatial hashing, good for large datasets
- Mesh mode: O(n²) triangulation, slower for >1000 points

### Optimization Tips
1. Batch point additions using `AddBitmapPoints()`
2. Disable real-time updates during bulk operations
3. Use voxel mode for very large point clouds
4. Implement LOD by adjusting `VoxelSize` based on distance

## AR/MR Platform Integration

### ARCore (Android)
```cpp
// In your ARCore component
void OnARFrameUpdated(const FARFrame& Frame)
{
    TArray<FVector> Points;
    TArray<FColor> Colors;
    
    // Get point cloud from ARCore
    const TArray<FVector>& ARPoints = Frame.GetPointCloud();
    Points = ARPoints;
    Colors.Init(FColor::White, ARPoints.Num());
    
    // Send to MRS3D
    MRS3DActor->ReceiveARData(Points, Colors);
}
```

### ARKit (iOS)
```cpp
// In your ARKit component
void OnARPointCloudUpdated(ARPointCloud* PointCloud)
{
    TArray<FVector> Points;
    for (int i = 0; i < PointCloud.count; i++)
    {
        Points.Add(FVector(PointCloud.points[i]));
    }
    
    TArray<FColor> Colors;
    Colors.Init(FColor::White, Points.Num());
    
    MRS3DActor->ReceiveARData(Points, Colors);
}
```

### HoloLens (Spatial Mapping)
```cpp
// In your spatial mapping component
void OnSpatialMappingUpdated(const FSpatialMappingData& Data)
{
    TArray<FVector> Points;
    TArray<FColor> Colors;
    
    // Convert spatial mesh to points
    for (const FVector& Vertex : Data.Vertices)
    {
        Points.Add(Vertex);
        Colors.Add(FColor::Cyan); // Visualize mapped surfaces
    }
    
    MRS3DActor->ReceiveARData(Points, Colors);
}
```

## Testing Without AR Hardware

The plugin includes simulation capabilities:

```cpp
// Simulate random AR input
MRS3DActor->SimulateARInput(1000, 500.0f);

// Or create specific patterns
TArray<FVector> TestPositions;
TArray<FColor> TestColors;

// Create a grid pattern
for (int x = 0; x < 10; x++)
{
    for (int y = 0; y < 10; y++)
    {
        TestPositions.Add(FVector(x * 50, y * 50, 0));
        TestColors.Add(FColor::Green);
    }
}

MRS3DActor->ReceiveARData(TestPositions, TestColors);
```

## Extending the System

### Custom Generation Algorithms
1. Add new enum value to `EProceduralGenerationType`
2. Implement generation function in `UProceduralGenerator`
3. Add switch case in `GenerateFromBitmapPoints()`

Example:
```cpp
void UProceduralGenerator::GenerateMarching Cubes(const TArray<FBitmapPoint>& Points)
{
    // Your custom algorithm here
}
```

### Custom Point Filters
```cpp
// In your gameplay code
UMRBitmapMapper* Mapper = GetSubsystem<UMRBitmapMapper>();
const TArray<FBitmapPoint>& AllPoints = Mapper->GetBitmapPoints();

// Filter by intensity
TArray<FBitmapPoint> HighIntensityPoints;
for (const FBitmapPoint& Point : AllPoints)
{
    if (Point.Intensity > 0.8f)
    {
        HighIntensityPoints.Add(Point);
    }
}

// Generate from filtered points
ProceduralGenerator->GenerateFromBitmapPoints(HighIntensityPoints);
```

## Debugging

### Enable Debug Visualization
```cpp
MRS3DActor->bEnableDebugVisualization = true;
```

### Log Output
The plugin logs to `LogTemp` category:
```
LogTemp: MRS3D Plugin Module Started
LogTemp: MRBitmapMapper Subsystem Initialized
LogTemp: Received 1000 bitmap points
```

### Console Commands
Add these to your project for debugging:

```cpp
// In your game mode or player controller
UFUNCTION(Exec)
void MRS3D_ClearPoints()
{
    UMRBitmapMapper* Mapper = GetWorld()->GetGameInstance()->GetSubsystem<UMRBitmapMapper>();
    Mapper->ClearBitmapPoints();
}

UFUNCTION(Exec)
void MRS3D_SimulateInput(int32 NumPoints)
{
    AMRS3DGameplayActor* Actor = FindActorOfClass<AMRS3DGameplayActor>();
    Actor->SimulateARInput(NumPoints, 500.0f);
}
```

## Best Practices

1. **Always batch point additions** - Use `AddBitmapPoints()` instead of multiple `AddBitmapPoint()` calls
2. **Configure update intervals** - Balance responsiveness vs. performance
3. **Use appropriate generation types** - Point cloud for debugging, voxel for large datasets, mesh for smooth surfaces
4. **Implement point lifetime management** - Remove old points to prevent memory issues
5. **Test with simulated data first** - Validate your implementation before connecting real AR hardware
6. **Profile your implementation** - Use UE5's profiling tools to identify bottlenecks

## Troubleshooting

### No geometry appears
- Check that points are being added: `Mapper->GetBitmapPoints().Num()`
- Verify `bAutoUpdate` is enabled
- Ensure actor is spawned and initialized
- Check point positions are within camera view

### Performance issues
- Reduce update frequency
- Use voxel mode instead of mesh
- Implement point culling
- Limit maximum points

### AR data not received
- Verify `bARDataReceptionEnabled` is true
- Check AR permissions on device
- Validate AR session is active
- Test with simulated input first

## API Reference

See the header files for complete API documentation:
- `BitmapPoint.h` - Data structures
- `MRBitmapMapper.h` - Subsystem API
- `ProceduralGenerator.h` - Generation component API
- `MRS3DGameplayActor.h` - Gameplay integration API
