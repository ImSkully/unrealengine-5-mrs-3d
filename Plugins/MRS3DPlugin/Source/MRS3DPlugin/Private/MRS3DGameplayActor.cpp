#include "MRS3DGameplayActor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

AMRS3DGameplayActor::AMRS3DGameplayActor()
	: bAutoGenerateOnReceive(true)
	, bEnableDebugVisualization(false)
	, bARDataReceptionEnabled(true)
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
