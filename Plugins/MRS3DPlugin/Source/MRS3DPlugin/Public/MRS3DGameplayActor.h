#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralGenerator.h"
#include "MRBitmapMapper.h"
#include "PlaneDetection.h"
#include "PlaneDetectionSubsystem.h"
#include "MRTrackingStateManager.h"
#include "MRS3DGameplayActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnARTrackingLoss, ETrackingState, PreviousState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnARTrackingRecovery, ETrackingState, NewState, float, LostDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnARTrackingQualityChanged, float, OldQuality, float, NewQuality);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpatialAnchorLost, FString, AnchorID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpatialAnchorRecovered, FString, AnchorID);

/**
 * Gameplay actor that integrates procedural generation with AR/MR systems
 */
UCLASS()
class FMRS3DPLUGIN_API AMRS3DGameplayActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AMRS3DGameplayActor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	/**
	 * Hook for receiving AR/MR data from external systems
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Gameplay")
	void ReceiveARData(const TArray<FVector>& Positions, const TArray<FColor>& Colors);

	/**
	 * Simulate AR/MR data input for testing
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Gameplay")
	void SimulateARInput(int32 NumPoints, float Radius);

	/**
	 * Enable/disable AR data reception
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Gameplay")
	void SetARDataReception(bool bEnabled);

	/**
	 * Update AR tracking state
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Gameplay")
	void UpdateTrackingState(ETrackingState NewState, float Quality = 1.0f, const FString& LossReason = TEXT(""));

	/**
	 * Handle AR tracking loss with automatic response
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Tracking")
	void HandleTrackingLoss(ETrackingState PreviousState, const FString& LossReason = TEXT(""));

	/**
	 * Handle AR tracking recovery with geometry restoration
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Tracking")
	void HandleTrackingRecovery(ETrackingState NewState, float LostDuration);

	/**
	 * Force spatial repositioning for tracking recovery
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Tracking")
	void ForceSpatialRepositioning();

	/**
	 * Set tracking loss response configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Tracking")
	void SetTrackingLossResponse(bool bPauseGeneration, bool bShowWarning, bool bAutoReposition);

	/**
	 * Get current tracking state information
	 */
	UFUNCTION(BlueprintPure, Category = "MRS3D|Tracking")
	ETrackingState GetCurrentTrackingState() const;

	/**
	 * Get tracking quality (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "MRS3D|Tracking")
	float GetTrackingQuality() const;

	/**
	 * Check if currently experiencing tracking loss
	 */
	UFUNCTION(BlueprintPure, Category = "MRS3D|Tracking")
	bool IsTrackingLost() const;

	/**
	 * Get time since tracking was lost (in seconds)
	 */
	UFUNCTION(BlueprintPure, Category = "MRS3D|Tracking")
	float GetTrackingLossDuration() const;

	/**
	 * Enable/disable plane detection
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Gameplay")
	void SetPlaneDetectionEnabled(bool bEnabled);

	/**
	 * Get all detected planes
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Gameplay")
	TArray<FDetectedPlane> GetDetectedPlanes() const;

	/**
	 * Get the largest detected floor plane
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Gameplay")
	FDetectedPlane GetLargestFloorPlane() const;

	/**
	 * Manually trigger plane detection
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Gameplay")
	void TriggerPlaneDetection();

	/**
	 * Simulate plane detection for testing
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|Gameplay")
	void SimulatePlaneDetection();

	/**
	 * Set marching cubes configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MarchingCubes")
	void SetMarchingCubesConfig(const FMarchingCubesConfig& Config);

	/**
	 * Generate mesh using marching cubes
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MarchingCubes")
	void GenerateWithMarchingCubes();

	/**
	 * Switch to marching cubes generation mode
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|MarchingCubes")
	void EnableMarchingCubesGeneration(bool bEnable = true);

	/**
	 * Generate mesh asynchronously for large datasets
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|AsyncGeneration")
	int32 GenerateAsyncMesh(bool bForceAsync = false);

	/**
	 * Cancel active async generation
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|AsyncGeneration")
	bool CancelAsyncGeneration();

	/**
	 * Get async generation progress
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|AsyncGeneration")
	float GetAsyncGenerationProgress() const;

	/**
	 * Check if async generation is active
	 */
	UFUNCTION(BlueprintCallable, Category = "MRS3D|AsyncGeneration")
	bool IsAsyncGenerationActive() const;

	/**
	 * Event handlers for async generation
	 */
	UFUNCTION()
	void OnAsyncGenerationComplete(bool bSuccess, int32 JobID);

	UFUNCTION()
	void OnAsyncGenerationProgress(int32 JobID, float Progress);

	/** AR Tracking Loss Events */
	UPROPERTY(BlueprintAssignable, Category = "Events|Tracking")
	FOnARTrackingLoss OnARTrackingLoss;

	UPROPERTY(BlueprintAssignable, Category = "Events|Tracking")
	FOnARTrackingRecovery OnARTrackingRecovery;

	UPROPERTY(BlueprintAssignable, Category = "Events|Tracking")
	FOnARTrackingQualityChanged OnARTrackingQualityChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Tracking")
	FOnSpatialAnchorLost OnSpatialAnchorLost;

	UPROPERTY(BlueprintAssignable, Category = "Events|Tracking")
	FOnSpatialAnchorRecovered OnSpatialAnchorRecovered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Gameplay")
	UProceduralGenerator* ProceduralGenerator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Gameplay")
	bool bAutoGenerateOnReceive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Gameplay")
	bool bEnableDebugVisualization;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|AsyncGeneration")
	bool bEnableAsyncGeneration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|AsyncGeneration")
	int32 AsyncGenerationThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|PlaneDetection")
	bool bAutoPlaneDetectionEnabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|PlaneDetection")
	bool bVisualizeDetectedPlanes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|PlaneDetection")
	float PlaneVisualizationDuration;

	/** AR Tracking Loss Configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Tracking")
	bool bPauseGenerationOnTrackingLoss;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Tracking")
	bool bShowTrackingLossWarning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Tracking")
	bool bAutoRepositionOnRecovery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Tracking")
	float TrackingQualityThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Tracking")
	float MaxTrackingLossTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Tracking")
	bool bUseSpatialAnchors;

protected:
	UPROPERTY()
	UMRBitmapMapper* BitmapMapper;

	bool bARDataReceptionEnabled;

	// Async generation tracking
	int32 CurrentAsyncJobID;
	bool bAsyncGenerationInProgress;

	// AR Tracking Loss Management
	ETrackingState CurrentTrackingState;
	float CurrentTrackingQuality;
	float TrackingLossStartTime;
	bool bIsCurrentlyTrackingLost;
	FString LastTrackingLossReason;
	FTransform PreLossActorTransform;
	UMRTrackingStateManager* TrackingStateManager;

	void OnBitmapPointsUpdated(const TArray<FBitmapPoint>& Points);
	
	// Plane detection event handlers
	UFUNCTION()
	void OnPlaneDetected(const FDetectedPlane& DetectedPlane);
	
	UFUNCTION()
	void OnPlaneUpdated(const FDetectedPlane& UpdatedPlane);
	
	UFUNCTION()
	void OnPlaneLost(const FString& PlaneID);
	
	UFUNCTION()
	void OnTrackingStateChanged(ETrackingState NewState);
	
	// Internal tracking event handlers
	UFUNCTION()
	void OnTrackingLostInternal(float LostDuration);
	
	UFUNCTION()
	void OnTrackingRecoveredInternal();
	
	UFUNCTION()
	void OnTrackingQualityChangedInternal(EMRTrackingQuality OldQuality, EMRTrackingQuality NewQuality);
	
	// ProceduralGenerator tracking event handlers
	UFUNCTION()
	void OnProceduralGeneratorTrackingLoss(ETrackingState PreviousState);
	
	UFUNCTION()
	void OnProceduralGeneratorTrackingRecovery(ETrackingState NewState, float LostDuration);
	
	UFUNCTION()
	void OnProceduralGeneratorTrackingQualityChange(float OldQuality, float NewQuality);
	
	// Helper methods
	void VisualizePlanes();
};
