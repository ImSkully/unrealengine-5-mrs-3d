# MRS3D Plugin Examples

## Example 1: Basic Point Cloud Visualization

This example shows how to visualize AR point cloud data as colored cubes.

### C++ Implementation

```cpp
// MyARComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MRS3DGameplayActor.h"
#include "MyARComponent.generated.h"

UCLASS()
class UMyARComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY()
    AMRS3DGameplayActor* MRS3DActor;
    
    void SimulateARPointCloud();
};

// MyARComponent.cpp
void UMyARComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Spawn MRS3D actor
    MRS3DActor = GetWorld()->SpawnActor<AMRS3DGameplayActor>();
    
    // Configure for point cloud
    MRS3DActor->ProceduralGenerator->SetGenerationType(EProceduralGenerationType::PointCloud);
    MRS3DActor->ProceduralGenerator->VoxelSize = 5.0f;
    MRS3DActor->bEnableDebugVisualization = true;
}

void UMyARComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    SimulateARPointCloud();
}

void UMyARComponent::SimulateARPointCloud()
{
    // Simulate moving point cloud
    TArray<FVector> Positions;
    TArray<FColor> Colors;
    
    float Time = GetWorld()->GetTimeSeconds();
    
    for (int i = 0; i < 500; i++)
    {
        float Angle = (i / 500.0f) * 2 * PI;
        float Radius = 200.0f + FMath::Sin(Time + Angle) * 50.0f;
        
        FVector Pos(
            FMath::Cos(Angle) * Radius,
            FMath::Sin(Angle) * Radius,
            FMath::Sin(Time * 2 + i * 0.1f) * 100.0f
        );
        
        Positions.Add(Pos);
        
        // Color based on height
        float Height = (Pos.Z + 100.0f) / 200.0f;
        FColor Color = FLinearColor(Height, 1.0f - Height, 0.5f, 1.0f).ToFColor(true);
        Colors.Add(Color);
    }
    
    MRS3DActor->ReceiveARData(Positions, Colors);
}
```

## Example 2: Real-Time Voxel Terrain Generation

Create terrain from AR spatial mapping data.

### Blueprint Setup

1. Create a new Blueprint based on `MRS3DGameplayActor`
2. Set these properties:
   - Generation Type: Voxel
   - Voxel Size: 25.0
   - Auto Update: true
   - Update Interval: 0.2

### C++ Integration with ARCore

```cpp
void AMyARManager::OnARSessionUpdated()
{
    UARSessionConfig* Config = UARBlueprintLibrary::GetSessionConfig();
    
    if (Config && Config->bGenerateMeshDataFromTrackedGeometry)
    {
        TArray<FARTraceResult> HitResults;
        UARBlueprintLibrary::LineTraceTrackedObjects(
            GetWorld(),
            CameraLocation,
            CameraLocation + CameraForward * 1000.0f,
            false,
            false,
            false,
            HitResults
        );
        
        TArray<FVector> SurfacePoints;
        TArray<FColor> SurfaceColors;
        
        for (const FARTraceResult& Hit : HitResults)
        {
            // Sample around hit point
            for (int x = -5; x <= 5; x++)
            {
                for (int y = -5; y <= 5; y++)
                {
                    FVector SamplePos = Hit.GetLocalToWorldTransform().GetLocation();
                    SamplePos += FVector(x * 10, y * 10, 0);
                    
                    SurfacePoints.Add(SamplePos);
                    SurfaceColors.Add(FColor::Green);
                }
            }
        }
        
        MRS3DActor->ReceiveARData(SurfacePoints, SurfaceColors);
    }
}
```

## Example 3: Interactive Spatial Mesh Editor

Allow users to paint in 3D space with AR.

```cpp
// ARPaintComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MRBitmapMapper.h"
#include "ARPaintComponent.generated.h"

UCLASS()
class UARPaintComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "AR Paint")
    void StartPainting(FColor PaintColor);
    
    UFUNCTION(BlueprintCallable, Category = "AR Paint")
    void StopPainting();
    
    UFUNCTION(BlueprintCallable, Category = "AR Paint")
    void Paint(FVector Location);
    
    UPROPERTY(EditAnywhere, Category = "AR Paint")
    float BrushSize = 10.0f;
    
    UPROPERTY(EditAnywhere, Category = "AR Paint")
    int32 PointsPerStroke = 5;

private:
    bool bIsPainting;
    FColor CurrentColor;
    UMRBitmapMapper* BitmapMapper;
    
    virtual void BeginPlay() override;
};

// ARPaintComponent.cpp
void UARPaintComponent::BeginPlay()
{
    Super::BeginPlay();
    BitmapMapper = GetWorld()->GetGameInstance()->GetSubsystem<UMRBitmapMapper>();
    bIsPainting = false;
}

void UARPaintComponent::StartPainting(FColor PaintColor)
{
    bIsPainting = true;
    CurrentColor = PaintColor;
}

void UARPaintComponent::StopPainting()
{
    bIsPainting = false;
}

void UARPaintComponent::Paint(FVector Location)
{
    if (!bIsPainting || !BitmapMapper) return;
    
    TArray<FBitmapPoint> PaintPoints;
    
    // Create multiple points around the brush location
    for (int i = 0; i < PointsPerStroke; i++)
    {
        FVector Offset = FMath::VRand() * FMath::FRandRange(0, BrushSize);
        FBitmapPoint Point;
        Point.Position = Location + Offset;
        Point.Color = CurrentColor;
        Point.Intensity = 1.0f;
        Point.Timestamp = GetWorld()->GetTimeSeconds();
        
        PaintPoints.Add(Point);
    }
    
    BitmapMapper->AddBitmapPoints(PaintPoints);
}
```

### Blueprint Usage

1. Add `ARPaintComponent` to your pawn
2. On touch/click:
   - Call `StartPainting(DesiredColor)`
   - Every frame: Call `Paint(HandLocation)` or `Paint(TouchWorldLocation)`
3. On release: Call `StopPainting()`

## Example 4: AR Furniture Placement with Point Cloud Collision

```cpp
void AFurniturePlacementManager::TryPlaceFurniture(AActor* Furniture)
{
    UMRBitmapMapper* Mapper = GetWorld()->GetGameInstance()->GetSubsystem<UMRBitmapMapper>();
    
    FVector PlacementLocation = GetDesiredPlacementLocation();
    
    // Check for nearby AR points (floor detection)
    TArray<FBitmapPoint> NearbyPoints = Mapper->GetBitmapPointsInRadius(
        PlacementLocation,
        100.0f // Search radius
    );
    
    if (NearbyPoints.Num() < 10)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough AR data for placement"));
        return;
    }
    
    // Calculate average floor height
    float AverageHeight = 0;
    for (const FBitmapPoint& Point : NearbyPoints)
    {
        AverageHeight += Point.Position.Z;
    }
    AverageHeight /= NearbyPoints.Num();
    
    // Snap furniture to floor
    FVector SnappedLocation = PlacementLocation;
    SnappedLocation.Z = AverageHeight;
    
    Furniture->SetActorLocation(SnappedLocation);
    
    UE_LOG(LogTemp, Log, TEXT("Furniture placed at Z=%f based on %d AR points"), 
        AverageHeight, NearbyPoints.Num());
}
```

## Example 5: Data Persistence

Save and load AR mapping data.

```cpp
// MRS3DSaveGame.h
#pragma once

#include "GameFramework/SaveGame.h"
#include "BitmapPoint.h"
#include "MRS3DSaveGame.generated.h"

UCLASS()
class UMRS3DSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TArray<FBitmapPoint> SavedBitmapPoints;
    
    UPROPERTY()
    FString SessionName;
    
    UPROPERTY()
    FDateTime SaveTime;
};

// Saving
void SaveARSession()
{
    UMRS3DSaveGame* SaveGame = Cast<UMRS3DSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UMRS3DSaveGame::StaticClass())
    );
    
    UMRBitmapMapper* Mapper = GetGameInstance()->GetSubsystem<UMRBitmapMapper>();
    SaveGame->SavedBitmapPoints = Mapper->GetBitmapPoints();
    SaveGame->SessionName = "ARSession_001";
    SaveGame->SaveTime = FDateTime::Now();
    
    UGameplayStatics::SaveGameToSlot(SaveGame, "ARSession", 0);
}

// Loading
void LoadARSession()
{
    UMRS3DSaveGame* LoadGame = Cast<UMRS3DSaveGame>(
        UGameplayStatics::LoadGameFromSlot("ARSession", 0)
    );
    
    if (LoadGame)
    {
        UMRBitmapMapper* Mapper = GetGameInstance()->GetSubsystem<UMRBitmapMapper>();
        Mapper->ClearBitmapPoints();
        Mapper->AddBitmapPoints(LoadGame->SavedBitmapPoints);
        
        UE_LOG(LogTemp, Log, TEXT("Loaded %d points from session: %s"), 
            LoadGame->SavedBitmapPoints.Num(),
            *LoadGame->SessionName);
    }
}
```

## Performance Tips for Each Example

### Example 1 (Point Cloud)
- Limit points to 1000-2000 for smooth 60fps
- Use larger VoxelSize for distant objects

### Example 2 (Voxel Terrain)
- VoxelSize of 25-50 units works well
- Update interval of 0.2-0.5s recommended

### Example 3 (AR Paint)
- Clear old points periodically
- Implement point culling based on age

### Example 4 (Furniture Placement)
- Cache point queries
- Use smaller search radius (50-100 units)

### Example 5 (Data Persistence)
- Compress saved data for large point clouds
- Implement point downsampling before save
