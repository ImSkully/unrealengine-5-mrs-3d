#include "PlaneDetectionSubsystem.h"
#include "Engine/Engine.h"
#include "Algo/Heap.h"

UPlaneDetectionSubsystem::UPlaneDetectionSubsystem()
	: bPlaneDetectionEnabled(true)
	, MinimumPlaneArea(0.25f) // 0.5m x 0.5m minimum
	, PlaneValidationInterval(5.0f)
	, MaxPlaneAge(60.0f)
	, bAutoValidatePlanes(true)
	, LastValidationTime(0.0f)
{
}

void UPlaneDetectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("PlaneDetectionSubsystem Initialized"));
	
	DetectedPlanes.Empty();
	CurrentTrackingSession = FTrackingSession();
	LastValidationTime = FPlatformTime::Seconds();
}

void UPlaneDetectionSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("PlaneDetectionSubsystem Deinitialized"));
	DetectedPlanes.Empty();
	Super::Deinitialize();
}

void UPlaneDetectionSubsystem::AddDetectedPlane(const FDetectedPlane& Plane)
{
	if (!bPlaneDetectionEnabled || !IsPlaneValid(Plane))
	{
		return;
	}

	FString PlaneID = Plane.PlaneID.IsEmpty() ? GeneratePlaneID() : Plane.PlaneID;
	
	// Check if plane already exists
	if (DetectedPlanes.Contains(PlaneID))
	{
		// Update existing plane
		FDetectedPlane& ExistingPlane = DetectedPlanes[PlaneID];
		ExistingPlane = Plane;
		ExistingPlane.PlaneID = PlaneID;
		ExistingPlane.UpdateTimestamp();
		
		OnPlaneUpdated.Broadcast(ExistingPlane);
		UE_LOG(LogTemp, Verbose, TEXT("Updated plane %s"), *PlaneID);
	}
	else
	{
		// Add new plane
		FDetectedPlane NewPlane = Plane;
		NewPlane.PlaneID = PlaneID;
		NewPlane.UpdateTimestamp();
		
		// Classify plane type if unknown
		if (NewPlane.PlaneType == EPlaneType::Unknown)
		{
			NewPlane.PlaneType = ClassifyPlane(NewPlane.Normal, NewPlane.Center);
		}
		
		DetectedPlanes.Add(PlaneID, NewPlane);
		OnPlaneDetected.Broadcast(NewPlane);
		UE_LOG(LogTemp, Log, TEXT("Added new plane %s of type %d"), *PlaneID, (int32)NewPlane.PlaneType);
	}

	// Perform auto validation if enabled
	if (bAutoValidatePlanes)
	{
		PerformAutoValidation();
	}
}

bool UPlaneDetectionSubsystem::UpdatePlane(const FString& PlaneID, const FDetectedPlane& UpdatedPlane)
{
	if (!DetectedPlanes.Contains(PlaneID))
	{
		return false;
	}

	FDetectedPlane& ExistingPlane = DetectedPlanes[PlaneID];
	
	// Preserve ID and update other properties
	FDetectedPlane NewPlane = UpdatedPlane;
	NewPlane.PlaneID = PlaneID;
	NewPlane.UpdateTimestamp();
	
	ExistingPlane = NewPlane;
	OnPlaneUpdated.Broadcast(ExistingPlane);
	
	return true;
}

bool UPlaneDetectionSubsystem::RemovePlane(const FString& PlaneID)
{
	if (DetectedPlanes.Contains(PlaneID))
	{
		DetectedPlanes.Remove(PlaneID);
		OnPlaneLost.Broadcast(PlaneID);
		UE_LOG(LogTemp, Log, TEXT("Removed plane %s"), *PlaneID);
		return true;
	}
	return false;
}

TArray<FDetectedPlane> UPlaneDetectionSubsystem::GetAllPlanes() const
{
	TArray<FDetectedPlane> AllPlanes;
	DetectedPlanes.GenerateValueArray(AllPlanes);
	return AllPlanes;
}

TArray<FDetectedPlane> UPlaneDetectionSubsystem::GetPlanesByType(EPlaneType PlaneType) const
{
	TArray<FDetectedPlane> FilteredPlanes;
	
	for (const auto& PlanesPair : DetectedPlanes)
	{
		if (PlanesPair.Value.PlaneType == PlaneType)
		{
			FilteredPlanes.Add(PlanesPair.Value);
		}
	}
	
	return FilteredPlanes;
}

FDetectedPlane UPlaneDetectionSubsystem::GetLargestPlane(EPlaneType PlaneType) const
{
	FDetectedPlane LargestPlane;
	float LargestArea = 0.0f;
	
	for (const auto& PlanesPair : DetectedPlanes)
	{
		const FDetectedPlane& Plane = PlanesPair.Value;
		if ((PlaneType == EPlaneType::Unknown || Plane.PlaneType == PlaneType) && Plane.GetArea() > LargestArea)
		{
			LargestArea = Plane.GetArea();
			LargestPlane = Plane;
		}
	}
	
	return LargestPlane;
}

TArray<FDetectedPlane> UPlaneDetectionSubsystem::GetPlanesInRadius(const FVector& Center, float Radius) const
{
	TArray<FDetectedPlane> PlanesInRadius;
	const float RadiusSquared = Radius * Radius;
	
	for (const auto& PlanesPair : DetectedPlanes)
	{
		const FDetectedPlane& Plane = PlanesPair.Value;
		if (FVector::DistSquared(Plane.Center, Center) <= RadiusSquared)
		{
			PlanesInRadius.Add(Plane);
		}
	}
	
	return PlanesInRadius;
}

FVector UPlaneDetectionSubsystem::ProjectPointToNearestPlane(const FVector& Point, EPlaneType PlaneType) const
{
	FDetectedPlane NearestPlane;
	float NearestDistance = FLT_MAX;
	
	for (const auto& PlanesPair : DetectedPlanes)
	{
		const FDetectedPlane& Plane = PlanesPair.Value;
		if (PlaneType != EPlaneType::Unknown && Plane.PlaneType != PlaneType)
		{
			continue;
		}
		
		// Calculate distance from point to plane
		float Distance = FMath::Abs(FVector::DotProduct(Point - Plane.Center, Plane.Normal));
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			NearestPlane = Plane;
		}
	}
	
	if (NearestPlane.PlaneID.IsEmpty())
	{
		return Point; // No plane found, return original point
	}
	
	// Project point onto the plane
	FVector ToPoint = Point - NearestPlane.Center;
	float DistanceToPlane = FVector::DotProduct(ToPoint, NearestPlane.Normal);
	return Point - (NearestPlane.Normal * DistanceToPlane);
}

void UPlaneDetectionSubsystem::UpdateTrackingState(ETrackingState NewState, float Quality, const FString& LossReason)
{
	ETrackingState PreviousState = CurrentTrackingSession.CurrentState;
	CurrentTrackingSession.UpdateState(NewState, Quality);
	
	if (!LossReason.IsEmpty())
	{
		CurrentTrackingSession.TrackingLossReason = LossReason;
	}
	
	if (PreviousState != NewState)
	{
		OnTrackingStateChanged.Broadcast(NewState);
		UE_LOG(LogTemp, Log, TEXT("Tracking state changed from %d to %d (Quality: %.2f)"), 
			(int32)PreviousState, (int32)NewState, Quality);
	}
	
	// Handle tracking loss - mark planes as not tracked
	if (NewState == ETrackingState::TrackingLost || NewState == ETrackingState::NotTracking)
	{
		for (auto& PlanesPair : DetectedPlanes)
		{
			PlanesPair.Value.bIsTracked = false;
		}
	}
	else if (NewState == ETrackingState::FullTracking)
	{
		// Re-enable tracking for all planes
		for (auto& PlanesPair : DetectedPlanes)
		{
			PlanesPair.Value.bIsTracked = true;
		}
	}
}

void UPlaneDetectionSubsystem::ClearAllPlanes()
{
	TArray<FString> PlaneIDs;
	DetectedPlanes.GetKeys(PlaneIDs);
	
	DetectedPlanes.Empty();
	
	// Broadcast lost events for all planes
	for (const FString& PlaneID : PlaneIDs)
	{
		OnPlaneLost.Broadcast(PlaneID);
	}
	
	UE_LOG(LogTemp, Log, TEXT("Cleared all detected planes"));
}

void UPlaneDetectionSubsystem::SetPlaneDetectionEnabled(bool bEnabled)
{
	bPlaneDetectionEnabled = bEnabled;
	UE_LOG(LogTemp, Log, TEXT("Plane detection %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void UPlaneDetectionSubsystem::SetMinimumPlaneSize(float MinArea)
{
	MinimumPlaneArea = FMath::Max(0.01f, MinArea);
	UE_LOG(LogTemp, Log, TEXT("Minimum plane area set to %.2f"), MinimumPlaneArea);
}

TArray<FDetectedPlane> UPlaneDetectionSubsystem::DetectPlanesFromPoints(const TArray<FBitmapPoint>& Points, float PlaneThickness)
{
	TArray<FDetectedPlane> DetectedPlanesFromPoints;
	
	if (Points.Num() < 3 || !bPlaneDetectionEnabled)
	{
		return DetectedPlanesFromPoints;
	}
	
	// Simple plane detection using RANSAC-like approach
	// This is a basic implementation - in production, you'd use more sophisticated algorithms
	
	TArray<FBitmapPoint> RemainingPoints = Points;
	int32 PlaneCount = 0;
	
	while (RemainingPoints.Num() >= 3 && PlaneCount < 10) // Limit to 10 planes max
	{
		FDetectedPlane BestPlane;
		TArray<int32> BestInliers;
		int32 BestInlierCount = 0;
		
		// RANSAC iterations
		for (int32 Iteration = 0; Iteration < 100; ++Iteration)
		{
			if (RemainingPoints.Num() < 3) break;
			
			// Pick 3 random points
			int32 Idx1 = FMath::RandRange(0, RemainingPoints.Num() - 1);
			int32 Idx2 = FMath::RandRange(0, RemainingPoints.Num() - 1);
			int32 Idx3 = FMath::RandRange(0, RemainingPoints.Num() - 1);
			
			if (Idx1 == Idx2 || Idx1 == Idx3 || Idx2 == Idx3) continue;
			
			FVector P1 = RemainingPoints[Idx1].Position;
			FVector P2 = RemainingPoints[Idx2].Position;
			FVector P3 = RemainingPoints[Idx3].Position;
			
			// Calculate plane normal
			FVector V1 = P2 - P1;
			FVector V2 = P3 - P1;
			FVector Normal = FVector::CrossProduct(V1, V2).GetSafeNormal();
			
			if (Normal.IsNearlyZero()) continue;
			
			// Find inliers
			TArray<int32> Inliers;
			FVector PlaneCenter = FVector::ZeroVector;
			
			for (int32 i = 0; i < RemainingPoints.Num(); ++i)
			{
				FVector Point = RemainingPoints[i].Position;
				float Distance = FMath::Abs(FVector::DotProduct(Point - P1, Normal));
				
				if (Distance <= PlaneThickness)
				{
					Inliers.Add(i);
					PlaneCenter += Point;
				}
			}
			
			if (Inliers.Num() > BestInlierCount && Inliers.Num() >= 10) // Minimum 10 points for a plane
			{
				BestInlierCount = Inliers.Num();
				BestInliers = Inliers;
				PlaneCenter /= Inliers.Num();
				
				// Calculate plane extent (simplified bounding box)
				FVector MinBounds = PlaneCenter;
				FVector MaxBounds = PlaneCenter;
				
				for (int32 InlierIdx : Inliers)
				{
					FVector Point = RemainingPoints[InlierIdx].Position;
					MinBounds = MinBounds.ComponentMin(Point);
					MaxBounds = MaxBounds.ComponentMax(Point);
				}
				
				FVector Size = MaxBounds - MinBounds;
				FVector2D Extent(Size.X * 0.5f, Size.Y * 0.5f);
				
				BestPlane = FDetectedPlane(GeneratePlaneID(), PlaneCenter, Normal, Extent);
				BestPlane.PlaneType = ClassifyPlane(Normal, PlaneCenter);
				BestPlane.Confidence = (BestInlierCount > 50) ? EPlaneConfidence::High : 
									   (BestInlierCount > 25) ? EPlaneConfidence::Medium : EPlaneConfidence::Low;
			}
		}
		
		// Add the best plane if it meets minimum requirements
		if (BestInlierCount >= 10 && BestPlane.GetArea() >= MinimumPlaneArea)
		{
			DetectedPlanesFromPoints.Add(BestPlane);
			
			// Remove inlier points for next iteration
			for (int32 i = BestInliers.Num() - 1; i >= 0; --i)
			{
				RemainingPoints.RemoveAt(BestInliers[i]);
			}
			
			PlaneCount++;
		}
		else
		{
			break; // No more valid planes found
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("Detected %d planes from %d points"), DetectedPlanesFromPoints.Num(), Points.Num());
	return DetectedPlanesFromPoints;
}

int32 UPlaneDetectionSubsystem::ValidatePlaneTracking(float MaxAge)
{
	const float CurrentTime = FPlatformTime::Seconds();
	TArray<FString> PlanesToRemove;
	
	for (const auto& PlanesPair : DetectedPlanes)
	{
		const FDetectedPlane& Plane = PlanesPair.Value;
		float Age = CurrentTime - Plane.LastUpdateTime;
		
		if (Age > MaxAge)
		{
			PlanesToRemove.Add(PlanesPair.Key);
		}
	}
	
	// Remove stale planes
	for (const FString& PlaneID : PlanesToRemove)
	{
		RemovePlane(PlaneID);
	}
	
	LastValidationTime = CurrentTime;
	
	if (PlanesToRemove.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Validated plane tracking: removed %d stale planes"), PlanesToRemove.Num());
	}
	
	return PlanesToRemove.Num();
}

void UPlaneDetectionSubsystem::PerformAutoValidation()
{
	const float CurrentTime = FPlatformTime::Seconds();
	
	if (CurrentTime - LastValidationTime >= PlaneValidationInterval)
	{
		ValidatePlaneTracking(MaxPlaneAge);
	}
}

EPlaneType UPlaneDetectionSubsystem::ClassifyPlane(const FVector& Normal, const FVector& Center) const
{
	// Classify based on normal vector orientation
	FVector AbsNormal = Normal.GetAbs();
	
	// Check if it's horizontal (normal pointing up/down)
	if (AbsNormal.Z > 0.8f)
	{
		if (Normal.Z > 0)
		{
			// Check height to distinguish floor from table
			if (Center.Z < 50.0f) // Less than 50cm high
			{
				return EPlaneType::Floor;
			}
			else if (Center.Z < 150.0f) // Between 50cm and 150cm
			{
				return EPlaneType::Table;
			}
			else
			{
				return EPlaneType::Horizontal;
			}
		}
		else
		{
			return EPlaneType::Ceiling;
		}
	}
	// Check if it's vertical (normal pointing horizontally)
	else if (AbsNormal.Z < 0.3f && (AbsNormal.X > 0.7f || AbsNormal.Y > 0.7f))
	{
		return EPlaneType::Wall;
	}
	// Angled surface
	else
	{
		return EPlaneType::Angled;
	}
}

FString UPlaneDetectionSubsystem::GeneratePlaneID() const
{
	static int32 PlaneCounter = 0;
	return FString::Printf(TEXT("Plane_%d_%d"), PlaneCounter++, FMath::RandRange(1000, 9999));
}

bool UPlaneDetectionSubsystem::IsPlaneValid(const FDetectedPlane& Plane) const
{
	// Check minimum area
	if (Plane.GetArea() < MinimumPlaneArea)
	{
		return false;
	}
	
	// Check normal vector validity
	if (Plane.Normal.IsNearlyZero() || !Plane.Normal.IsNormalized())
	{
		return false;
	}
	
	// Check extent validity
	if (Plane.Extent.X <= 0.0f || Plane.Extent.Y <= 0.0f)
	{
		return false;
	}
	
	return true;
}