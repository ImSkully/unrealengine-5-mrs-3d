# unrealengine-5-mrs-3d
Mixed Reality Spatial 3D Procedural Generation System for Unreal Engine 5

## Overview
This project provides a complete procedural generation system for Unreal Engine 5 that integrates with Mixed Reality (MR) and Augmented Reality (AR) systems to map bitmap points into the engine in real-time. The system hooks into Unreal's native gameplay framework, allowing seamless integration with existing projects.

## Features

### Core Capabilities
- **Real-time Bitmap Mapping**: Map AR/MR bitmap points into the Unreal Engine environment in real-time
- **Procedural Generation**: Multiple generation modes for creating 3D geometry from point cloud data
- **Gameplay Integration**: Native hooks into Unreal Engine's gameplay framework
- **Platform Support**: Works on Windows, Android, iOS, and HoloLens

### Procedural Generation Modes
1. **Point Cloud**: Renders individual points as small cubes
2. **Mesh**: Creates triangulated meshes from point data
3. **Voxel**: Generates voxel-based geometry on a grid
4. **Surface**: Creates continuous surfaces from point clouds

## Project Structure

```
MRS3D/
├── MRS3D.uproject              # Main Unreal Engine 5 project file
├── Source/
│   ├── MRS3D/                  # Main game module
│   │   ├── Public/
│   │   ├── Private/
│   │   └── MRS3D.Build.cs
│   ├── MRS3DTarget.cs          # Build target configuration
│   └── MRS3DEditorTarget.cs    # Editor build target
└── Plugins/
    └── MRS3DPlugin/            # Core plugin
        ├── MRS3DPlugin.uplugin # Plugin descriptor
        ├── Config/
        │   └── DefaultMRS3DPlugin.ini
        └── Source/
            └── MRS3DPlugin/
                ├── Public/
                │   ├── BitmapPoint.h           # Bitmap point data structure
                │   ├── MRBitmapMapper.h        # AR/MR bitmap mapping subsystem
                │   ├── ProceduralGenerator.h   # Procedural generation component
                │   └── MRS3DGameplayActor.h    # Gameplay integration actor
                └── Private/
                    ├── MRBitmapMapper.cpp
                    ├── ProceduralGenerator.cpp
                    └── MRS3DGameplayActor.cpp
```

## Quick Start

### Prerequisites
- Unreal Engine 5.0 or later
- Visual Studio 2019/2022 (Windows) or Xcode (Mac)
- For AR/MR features: Compatible AR/MR device or simulator

### Setup
1. Clone this repository
2. Right-click on `MRS3D.uproject` and select "Generate Visual Studio project files"
3. Open the generated solution file
4. Build the project in your IDE
5. Launch the project in Unreal Editor

### Basic Usage

#### In Blueprints
1. Add an `MRS3DGameplayActor` to your level
2. Configure the procedural generator settings in the details panel
3. Call `ReceiveARData` to send AR/MR point data to the system
4. The geometry will be generated automatically

#### In C++
```cpp
// Get the bitmap mapper subsystem
UMRBitmapMapper* Mapper = GetWorld()->GetGameInstance()->GetSubsystem<UMRBitmapMapper>();

// Create bitmap points from your AR/MR data
TArray<FBitmapPoint> Points;
FBitmapPoint Point;
Point.Position = FVector(100, 100, 0);
Point.Color = FColor::Red;
Points.Add(Point);

// Add points to the mapper
Mapper->AddBitmapPoints(Points);
```

## Component Reference

### UMRBitmapMapper (Subsystem)
Game instance subsystem that manages AR/MR bitmap points.

**Key Functions:**
- `AddBitmapPoint(const FBitmapPoint& Point)` - Add a single point
- `AddBitmapPoints(const TArray<FBitmapPoint>& Points)` - Add multiple points
- `GetBitmapPoints()` - Retrieve all stored points
- `GetBitmapPointsInRadius(FVector Center, float Radius)` - Get points in radius
- `ClearBitmapPoints()` - Clear all points
- `SetRealTimeUpdates(bool bEnabled)` - Enable/disable real-time updates

**Events:**
- `OnBitmapPointsUpdated` - Fired when bitmap points are updated

### UProceduralGenerator (Component)
Component that generates 3D geometry from bitmap points.

**Properties:**
- `GenerationType` - Type of procedural generation (PointCloud, Mesh, Voxel, Surface)
- `VoxelSize` - Size of voxels in voxel mode
- `bAutoUpdate` - Automatically update geometry when points change
- `UpdateInterval` - Frequency of auto-updates (seconds)

**Key Functions:**
- `GenerateFromBitmapPoints(const TArray<FBitmapPoint>& Points)` - Generate geometry
- `UpdateGeometry(const TArray<FBitmapPoint>& Points)` - Update existing geometry
- `ClearGeometry()` - Clear all generated geometry
- `SetGenerationType(EProceduralGenerationType NewType)` - Change generation mode

### AMRS3DGameplayActor (Actor)
Gameplay actor that integrates the procedural generation system with the game world.

**Properties:**
- `ProceduralGenerator` - Reference to the procedural generator component
- `bAutoGenerateOnReceive` - Auto-generate when receiving AR data
- `bEnableDebugVisualization` - Show debug visualization of points

**Key Functions:**
- `ReceiveARData(const TArray<FVector>& Positions, const TArray<FColor>& Colors)` - Receive AR/MR data
- `SimulateARInput(int32 NumPoints, float Radius)` - Simulate AR input for testing
- `SetARDataReception(bool bEnabled)` - Enable/disable AR data reception

## Example Workflow

### Testing with Simulated Data
```cpp
// In your game code or Blueprint
AMRS3DGameplayActor* Actor = GetWorld()->SpawnActor<AMRS3DGameplayActor>();
Actor->ProceduralGenerator->SetGenerationType(EProceduralGenerationType::Voxel);
Actor->ProceduralGenerator->VoxelSize = 50.0f;
Actor->SimulateARInput(1000, 500.0f); // Generate 1000 random points in 500 unit radius
```

### Receiving Real AR Data
```cpp
// Your AR system callback
void OnARPointCloudReceived(const TArray<FVector>& ARPoints)
{
    TArray<FColor> Colors;
    Colors.Init(FColor::White, ARPoints.Num());
    
    AMRS3DGameplayActor* Actor = FindActorOfClass<AMRS3DGameplayActor>();
    if (Actor)
    {
        Actor->ReceiveARData(ARPoints, Colors);
    }
}
```

## Configuration

Edit `Plugins/MRS3DPlugin/Config/DefaultMRS3DPlugin.ini` to configure default settings:
- Real-time update behavior
- Maximum bitmap points
- Default voxel sizes
- Update intervals
- Default generation types

## Platform-Specific Notes

### Windows
- Full functionality supported
- Use for development and testing

### Android/iOS
- AR Foundation integration recommended
- Configure AR session in project settings
- Ensure AR permissions are set in manifest/plist

### HoloLens
- Windows Mixed Reality plugin required
- Spatial mapping data can be fed directly to the system
- Configure hand tracking for interactive placement
- **Note**: For HoloLens 2 devices, update platform target to "HoloLens2" in .uproject and .uplugin files

## License
MIT License - See LICENSE file for details

## Contributing
Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## Support
For issues and questions, please open an issue on the GitHub repository.
