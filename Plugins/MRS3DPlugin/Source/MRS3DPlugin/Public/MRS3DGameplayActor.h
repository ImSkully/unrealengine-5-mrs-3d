#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralGenerator.h"
#include "MRBitmapMapper.h"
#include "PlaneDetection.h"
#include "PlaneDetectionSubsystem.h"
#include "MRS3DGameplayActor.generated.h"

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

protected:
	UPROPERTY()
	UMRBitmapMapper* BitmapMapper;

	bool bARDataReceptionEnabled;

	// Async generation tracking
	int32 CurrentAsyncJobID;
	bool bAsyncGenerationInProgress;

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
	
	// Helper methods
	void VisualizePlanes();
};
