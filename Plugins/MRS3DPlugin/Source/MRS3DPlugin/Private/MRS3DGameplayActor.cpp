#include "MRS3DGameplayActor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

AMRS3DGameplayActor::AMRS3DGameplayActor()
	: bAutoGenerateOnReceive(true)
	, bEnableDebugVisualization(false)
	, bARDataReceptionEnabled(true)
	, bAutoPlaneDetectionEnabled(false)
	, bVisualizeDetectedPlanes(true)
	, PlaneVisualizationDuration(5.0f)
{
	PrimaryActorTick.bCanEverTick = true;

	// Create procedural generator component
	ProceduralGenerator = CreateDefaultSubobject<UProceduralGenerator>(TEXT("ProceduralGenerator"));
}

void AMRS3DGameplayActor::BeginPlay()
{
	Super::BeginPlay();
	
	// Get the bitmap mapper subsystem
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		BitmapMapper = GameInstance->GetSubsystem<UMRBitmapMapper>();
		
		if (BitmapMapper)
		{
			BitmapMapper->OnBitmapPointsUpdated.AddDynamic(this, &AMRS3DGameplayActor::OnBitmapPointsUpdated);
			BitmapMapper->SetAutoPlaneDetectionEnabled(bAutoPlaneDetectionEnabled);
		}
		
		// Get plane detection subsystem and bind events
		if (UPlaneDetectionSubsystem* PlaneSubsystem = GameInstance->GetSubsystem<UPlaneDetectionSubsystem>())
		{
			PlaneSubsystem->OnPlaneDetected.AddDynamic(this, &AMRS3DGameplayActor::OnPlaneDetected);
			PlaneSubsystem->OnPlaneUpdated.AddDynamic(this, &AMRS3DGameplayActor::OnPlaneUpdated);
			PlaneSubsystem->OnPlaneLost.AddDynamic(this, &AMRS3DGameplayActor::OnPlaneLost);
			PlaneSubsystem->OnTrackingStateChanged.AddDynamic(this, &AMRS3DGameplayActor::OnTrackingStateChanged);
		}
	}
}

void AMRS3DGameplayActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bEnableDebugVisualization && BitmapMapper)
	{
		const TArray<FBitmapPoint>& Points = BitmapMapper->GetBitmapPoints();
		for (const FBitmapPoint& Point : Points)
		{
			DrawDebugSphere(GetWorld(), Point.Position, 5.0f, 8, Point.Color, false, -1.0f, 0, 1.0f);
		}
	}
	
	// Visualize detected planes
	if (bVisualizeDetectedPlanes)
	{
		VisualizePlanes();
	}
}

void AMRS3DGameplayActor::ReceiveARData(const TArray<FVector>& Positions, const TArray<FColor>& Colors)
{
	if (!bARDataReceptionEnabled || !BitmapMapper)
	{
		return;
	}

	TArray<FBitmapPoint> NewPoints;
	
	for (int32 i = 0; i < Positions.Num(); i++)
	{
		FBitmapPoint Point;
		Point.Position = Positions[i];
		Point.Color = i < Colors.Num() ? Colors[i] : FColor::White;
		Point.Timestamp = GetWorld()->GetTimeSeconds();
		NewPoints.Add(Point);
	}

	BitmapMapper->AddBitmapPoints(NewPoints);

	if (bAutoGenerateOnReceive && ProceduralGenerator)
	{
		ProceduralGenerator->GenerateFromBitmapPoints(NewPoints);
	}
}

void AMRS3DGameplayActor::SimulateARInput(int32 NumPoints, float Radius)
{
	TArray<FVector> Positions;
	TArray<FColor> Colors;

	for (int32 i = 0; i < NumPoints; i++)
	{
		// Generate random points in a sphere
		FVector RandomPoint = FMath::VRand() * FMath::FRandRange(0.0f, Radius);
		Positions.Add(GetActorLocation() + RandomPoint);
		
		// Generate random color
		Colors.Add(FColor::MakeRandomColor());
	}

	ReceiveARData(Positions, Colors);
}

void AMRS3DGameplayActor::SetARDataReception(bool bEnabled)
{
	bARDataReceptionEnabled = bEnabled;
}

void AMRS3DGameplayActor::OnBitmapPointsUpdated(const TArray<FBitmapPoint>& Points)
{
	if (bAutoGenerateOnReceive && ProceduralGenerator)
	{
		ProceduralGenerator->UpdateGeometry(Points);
	}
}

void AMRS3DGameplayActor::UpdateTrackingState(ETrackingState NewState, float Quality, const FString& LossReason)
{
	if (BitmapMapper)
	{
		BitmapMapper->UpdateARTrackingState(NewState, Quality, LossReason);
	}
}

void AMRS3DGameplayActor::SetPlaneDetectionEnabled(bool bEnabled)
{
	bAutoPlaneDetectionEnabled = bEnabled;
	
	if (BitmapMapper)
	{
		BitmapMapper->SetAutoPlaneDetectionEnabled(bEnabled);
	}
	
	UE_LOG(LogTemp, Log, TEXT("Plane detection %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

TArray<FDetectedPlane> AMRS3DGameplayActor::GetDetectedPlanes() const
{
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UPlaneDetectionSubsystem* PlaneSubsystem = GameInstance->GetSubsystem<UPlaneDetectionSubsystem>())
		{
			return PlaneSubsystem->GetAllPlanes();
		}
	}
	
	return TArray<FDetectedPlane>();
}

FDetectedPlane AMRS3DGameplayActor::GetLargestFloorPlane() const
{
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UPlaneDetectionSubsystem* PlaneSubsystem = GameInstance->GetSubsystem<UPlaneDetectionSubsystem>())
		{
			return PlaneSubsystem->GetLargestPlane(EPlaneType::Floor);
		}
	}
	
	return FDetectedPlane();
}

void AMRS3DGameplayActor::TriggerPlaneDetection()
{
	if (BitmapMapper)
	{
		TArray<FDetectedPlane> DetectedPlanes = BitmapMapper->DetectPlanesFromCurrentPoints();
		UE_LOG(LogTemp, Log, TEXT("Manual plane detection found %d planes"), DetectedPlanes.Num());
	}
}

void AMRS3DGameplayActor::SimulatePlaneDetection()
{
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UPlaneDetectionSubsystem* PlaneSubsystem = GameInstance->GetSubsystem<UPlaneDetectionSubsystem>())
		{
			// Simulate a floor plane
			FDetectedPlane FloorPlane;
			FloorPlane.PlaneID = TEXT("SimulatedFloor");
			FloorPlane.Center = GetActorLocation() + FVector(0, 0, -100);
			FloorPlane.Normal = FVector::UpVector;
			FloorPlane.Extent = FVector2D(200, 200); // 4m x 4m floor
			FloorPlane.PlaneType = EPlaneType::Floor;
			FloorPlane.Confidence = EPlaneConfidence::High;
			FloorPlane.bIsTracked = true;
			
			PlaneSubsystem->AddDetectedPlane(FloorPlane);
			
			// Simulate a wall plane
			FDetectedPlane WallPlane;
			WallPlane.PlaneID = TEXT("SimulatedWall");
			WallPlane.Center = GetActorLocation() + FVector(300, 0, 0);
			WallPlane.Normal = FVector(-1, 0, 0);
			WallPlane.Extent = FVector2D(150, 200); // 3m wide x 4m tall wall
			WallPlane.PlaneType = EPlaneType::Wall;
			WallPlane.Confidence = EPlaneConfidence::Medium;
			WallPlane.bIsTracked = true;
			
			PlaneSubsystem->AddDetectedPlane(WallPlane);
			
			UE_LOG(LogTemp, Log, TEXT("Simulated plane detection: added floor and wall planes"));
		}
	}
}

// Plane detection event handlers
UFUNCTION()
void AMRS3DGameplayActor::OnPlaneDetected(const FDetectedPlane& DetectedPlane)
{
	UE_LOG(LogTemp, Log, TEXT("Plane detected: %s (Type: %d, Area: %.2f)"), 
		*DetectedPlane.PlaneID, (int32)DetectedPlane.PlaneType, DetectedPlane.GetArea());
}

UFUNCTION()
void AMRS3DGameplayActor::OnPlaneUpdated(const FDetectedPlane& UpdatedPlane)
{
	UE_LOG(LogTemp, Verbose, TEXT("Plane updated: %s"), *UpdatedPlane.PlaneID);
}

UFUNCTION()
void AMRS3DGameplayActor::OnPlaneLost(const FString& PlaneID)
{
	UE_LOG(LogTemp, Log, TEXT("Plane lost: %s"), *PlaneID);
}

UFUNCTION()
void AMRS3DGameplayActor::OnTrackingStateChanged(ETrackingState NewState)
{
	UE_LOG(LogTemp, Log, TEXT("Tracking state changed to: %d"), (int32)NewState);
}

void AMRS3DGameplayActor::VisualizePlanes()
{
	TArray<FDetectedPlane> DetectedPlanes = GetDetectedPlanes();
	
	for (const FDetectedPlane& Plane : DetectedPlanes)
	{
		if (!Plane.bIsTracked)
		{
			continue;
		}
		
		// Choose color based on plane type
		FColor PlaneColor = FColor::White;
		switch (Plane.PlaneType)
		{
			case EPlaneType::Floor:
				PlaneColor = FColor::Green;
				break;
			case EPlaneType::Wall:
				PlaneColor = FColor::Blue;
				break;
			case EPlaneType::Ceiling:
				PlaneColor = FColor::Yellow;
				break;
			case EPlaneType::Table:
				PlaneColor = FColor::Orange;
				break;
			default:
				PlaneColor = FColor::Purple;
				break;
		}
		
		// Draw plane as a wireframe rectangle
		FVector Corner1 = Plane.Center + FVector(-Plane.Extent.X, -Plane.Extent.Y, 0);
		FVector Corner2 = Plane.Center + FVector(Plane.Extent.X, -Plane.Extent.Y, 0);
		FVector Corner3 = Plane.Center + FVector(Plane.Extent.X, Plane.Extent.Y, 0);
		FVector Corner4 = Plane.Center + FVector(-Plane.Extent.X, Plane.Extent.Y, 0);
		
		// Draw wireframe rectangle
		DrawDebugLine(GetWorld(), Corner1, Corner2, PlaneColor, false, PlaneVisualizationDuration, 0, 2.0f);
		DrawDebugLine(GetWorld(), Corner2, Corner3, PlaneColor, false, PlaneVisualizationDuration, 0, 2.0f);
		DrawDebugLine(GetWorld(), Corner3, Corner4, PlaneColor, false, PlaneVisualizationDuration, 0, 2.0f);
		DrawDebugLine(GetWorld(), Corner4, Corner1, PlaneColor, false, PlaneVisualizationDuration, 0, 2.0f);
		
		// Draw normal vector
		DrawDebugDirectionalArrow(GetWorld(), Plane.Center, Plane.Center + (Plane.Normal * 50.0f), 
			5.0f, PlaneColor, false, PlaneVisualizationDuration, 0, 2.0f);
		
		// Draw plane center
		DrawDebugSphere(GetWorld(), Plane.Center, 10.0f, 8, PlaneColor, false, PlaneVisualizationDuration, 0, 2.0f);
	}
}

void AMRS3DGameplayActor::SetMarchingCubesConfig(const FMarchingCubesConfig& Config)
{
	if (ProceduralGenerator)
	{
		ProceduralGenerator->SetMarchingCubesConfig(Config);
		UE_LOG(LogTemp, Log, TEXT("Marching cubes configuration updated"));
	}
}

void AMRS3DGameplayActor::GenerateWithMarchingCubes()
{
	if (!ProceduralGenerator || !BitmapMapper)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot generate with marching cubes: missing components"));
		return;
	}
	
	const TArray<FBitmapPoint>& Points = BitmapMapper->GetBitmapPoints();
	if (Points.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No bitmap points available for marching cubes generation"));
		return;
	}
	
	ProceduralGenerator->GenerateMarchingCubes(Points);
	UE_LOG(LogTemp, Log, TEXT("Generated mesh using marching cubes from %d points"), Points.Num());
}

void AMRS3DGameplayActor::EnableMarchingCubesGeneration(bool bEnable)
{
	if (ProceduralGenerator)
	{
		if (bEnable)
		{
			ProceduralGenerator->SetGenerationType(EProceduralGenerationType::MarchingCubes);
			UE_LOG(LogTemp, Log, TEXT("Enabled marching cubes generation mode"));
		}
		else
		{
			ProceduralGenerator->SetGenerationType(EProceduralGenerationType::Mesh);
			UE_LOG(LogTemp, Log, TEXT("Disabled marching cubes generation mode"));
		}
	}
}
