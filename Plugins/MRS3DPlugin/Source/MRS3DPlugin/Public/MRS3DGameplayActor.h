#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralGenerator.h"
#include "MRBitmapMapper.h"
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Gameplay")
	UProceduralGenerator* ProceduralGenerator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Gameplay")
	bool bAutoGenerateOnReceive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MRS3D|Gameplay")
	bool bEnableDebugVisualization;

protected:
	UPROPERTY()
	UMRBitmapMapper* BitmapMapper;

	bool bARDataReceptionEnabled;

	void OnBitmapPointsUpdated(const TArray<FBitmapPoint>& Points);
};
