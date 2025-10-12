# MRS3D Quick Reference Card

## Quick Start (5 Minutes)

### 1. Setup Project
```bash
# Generate project files
Right-click MRS3D.uproject → "Generate Visual Studio project files"

# Build
Open MRS3D.sln → Build Solution

# Run
Launch from Visual Studio or open MRS3D.uproject
```

### 2. Spawn Actor (Blueprint)
```
1. Open level in editor
2. Place Actors panel → Search "MRS3DGameplayActor"
3. Drag into viewport
4. Select actor → Details panel → Configure properties
```

### 3. Test (No AR Hardware)
```
Select MRS3DGameplayActor in World Outliner
Blueprint → Call "SimulateARInput"
  - NumPoints: 1000
  - Radius: 500.0
```

## Common Code Snippets

### C++: Basic Usage
```cpp
// Get subsystem
UMRBitmapMapper* Mapper = 
    GetWorld()->GetGameInstance()->GetSubsystem<UMRBitmapMapper>();

// Add points
TArray<FBitmapPoint> Points;
FBitmapPoint Point;
Point.Position = FVector(100, 100, 0);
Point.Color = FColor::Red;
Points.Add(Point);
Mapper->AddBitmapPoints(Points);
```

### C++: Spawn Gameplay Actor
```cpp
AMRS3DGameplayActor* Actor = 
    GetWorld()->SpawnActor<AMRS3DGameplayActor>();
Actor->ProceduralGenerator->SetGenerationType(
    EProceduralGenerationType::Voxel);
Actor->ProceduralGenerator->VoxelSize = 25.0f;
```

### C++: Receive AR Data
```cpp
void OnARDataReceived(const TArray<FVector>& Positions)
{
    TArray<FColor> Colors;
    Colors.Init(FColor::White, Positions.Num());
    MRS3DActor->ReceiveARData(Positions, Colors);
}
```

### Blueprint: Generate from Points
```
Event BeginPlay
  → Spawn Actor (MRS3DGameplayActor)
  → Set Generation Type (Mesh/Voxel/PointCloud/Surface)
  → Call Simulate AR Input (NumPoints=500, Radius=300)
```

## Configuration Files

### DefaultMRS3DPlugin.ini
```ini
[/Script/MRS3DPlugin.MRBitmapMapper]
bDefaultRealTimeUpdates=true
MaxBitmapPoints=100000

[/Script/MRS3DPlugin.ProceduralGenerator]
DefaultVoxelSize=10.0
DefaultUpdateInterval=0.1
bDefaultAutoUpdate=true
```

## Component Properties

### UProceduralGenerator
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| GenerationType | Enum | Mesh | Point Cloud/Mesh/Voxel/Surface |
| VoxelSize | float | 10.0 | Size of voxels (voxel mode) |
| bAutoUpdate | bool | true | Auto-update on point changes |
| UpdateInterval | float | 0.1 | Seconds between updates |

### AMRS3DGameplayActor
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| bAutoGenerateOnReceive | bool | true | Auto-generate when receiving AR data |
| bEnableDebugVisualization | bool | false | Show debug spheres for points |

## Generation Modes

| Mode | Use Case | Performance | Best For |
|------|----------|-------------|----------|
| Point Cloud | Debugging, visualization | Fast (O(n)) | <1000 points |
| Mesh | Smooth surfaces | Slow (O(n²)) | <500 points |
| Voxel | Large datasets | Fast (O(n)) | 1000+ points |
| Surface | Continuous surfaces | Medium | Various |

## API Cheat Sheet

### UMRBitmapMapper (Subsystem)
```cpp
AddBitmapPoint(const FBitmapPoint& Point)
AddBitmapPoints(const TArray<FBitmapPoint>& Points)
GetBitmapPoints() → const TArray<FBitmapPoint>&
GetBitmapPointsInRadius(FVector Center, float Radius) → TArray<FBitmapPoint>
ClearBitmapPoints()
SetRealTimeUpdates(bool bEnabled)
```

### UProceduralGenerator (Component)
```cpp
GenerateFromBitmapPoints(const TArray<FBitmapPoint>& Points)
UpdateGeometry(const TArray<FBitmapPoint>& Points)
ClearGeometry()
SetGenerationType(EProceduralGenerationType NewType)
```

### AMRS3DGameplayActor
```cpp
ReceiveARData(const TArray<FVector>& Positions, const TArray<FColor>& Colors)
SimulateARInput(int32 NumPoints, float Radius)
SetARDataReception(bool bEnabled)
```

## Platform Integration

### ARCore (Android)
```cpp
#include "ARBlueprintLibrary.h"

TArray<FARTraceResult> Results;
UARBlueprintLibrary::LineTraceTrackedObjects(...);
// Extract points from Results
MRS3DActor->ReceiveARData(Points, Colors);
```

### ARKit (iOS)
```cpp
// In AR session update
const TArray<FVector>& ARPoints = ARSession->GetPointCloud();
MRS3DActor->ReceiveARData(ARPoints, Colors);
```

### HoloLens
```cpp
#include "WindowsMixedRealityFunctionLibrary.h"

// Get spatial mapping data
FSpatialMeshData MeshData = GetSpatialMeshData();
// Convert to points
MRS3DActor->ReceiveARData(MeshPoints, Colors);
```

## Debugging

### Enable Logging
```cpp
// In DefaultEngine.ini
[Core.Log]
LogTemp=Verbose

// Output
LogTemp: MRS3D Plugin Module Started
LogTemp: MRBitmapMapper Subsystem Initialized
LogTemp: Received 1000 bitmap points
```

### Debug Visualization
```cpp
MRS3DActor->bEnableDebugVisualization = true;
```

### Console Commands
```cpp
// Add to your PlayerController or GameMode
UFUNCTION(Exec)
void MRS3D_Clear()
{
    GetWorld()->GetGameInstance()
        ->GetSubsystem<UMRBitmapMapper>()
        ->ClearBitmapPoints();
}

UFUNCTION(Exec)
void MRS3D_Test(int32 Num)
{
    FindActorOfClass<AMRS3DGameplayActor>()
        ->SimulateARInput(Num, 500.0f);
}
```

## Performance Tips

### Optimize Point Count
```cpp
// Downsample if too many points
if (Points.Num() > 10000)
{
    TArray<FBitmapPoint> Downsampled;
    for (int i = 0; i < Points.Num(); i += 2)
        Downsampled.Add(Points[i]);
    Mapper->AddBitmapPoints(Downsampled);
}
```

### Batch Updates
```cpp
// Good
Mapper->SetRealTimeUpdates(false);
for (batch : batches)
    Mapper->AddBitmapPoints(batch);
Mapper->SetRealTimeUpdates(true);

// Bad
for (point : points)
    Mapper->AddBitmapPoint(point); // Broadcasts each time!
```

### Use Appropriate Mode
```cpp
// Large datasets
Generator->SetGenerationType(EProceduralGenerationType::Voxel);
Generator->VoxelSize = 50.0f; // Larger = fewer voxels

// Small datasets, need detail
Generator->SetGenerationType(EProceduralGenerationType::Mesh);
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| No geometry appears | Check point count > 0, bAutoUpdate = true |
| Low FPS | Reduce point count, increase UpdateInterval |
| Geometry at wrong location | Check point coordinate system/scale |
| AR data not received | Verify AR permissions, session active |
| Crash on large dataset | Reduce MaxBitmapPoints in INI |

## File Locations

```
Important Files:
├── MRS3D.uproject (Project file)
├── README.md (Full documentation)
├── DEVELOPER_GUIDE.md (Architecture details)
├── EXAMPLES.md (Code samples)
│
Plugin Code:
├── Plugins/MRS3DPlugin/Source/MRS3DPlugin/
│   ├── Public/
│   │   ├── BitmapPoint.h (Data structures)
│   │   ├── MRBitmapMapper.h (Subsystem)
│   │   ├── ProceduralGenerator.h (Generator)
│   │   └── MRS3DGameplayActor.h (Actor)
│   └── Private/ (Implementations)
│
Configuration:
└── Plugins/MRS3DPlugin/Config/DefaultMRS3DPlugin.ini
```

## Next Steps

1. **Read**: DEVELOPER_GUIDE.md for architecture
2. **Try**: EXAMPLES.md for code samples
3. **Explore**: ProceduralGenerator.cpp for generation algorithms
4. **Extend**: Add custom generation modes or filters
5. **Integrate**: Connect your AR/MR device

## Support

- GitHub Issues: Report bugs and request features
- Documentation: README.md, DEVELOPER_GUIDE.md, EXAMPLES.md
- Code Comments: All public APIs are documented

---
**MRS3D v1.0** - Mixed Reality Spatial 3D Procedural Generation
