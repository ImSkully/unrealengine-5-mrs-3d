// Copyright (c) MIT License

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObstacleBase.generated.h"

/**
 * Base class for obstacles with fixed positions that spawn from the ground.
 * Features rising and falling animations implemented in Blueprint.
 * Note: Currently not in use.
 */
UCLASS()
class MYPROJECT_API AObstacleBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AObstacleBase();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	/** Triggers the obstacle's rising animation */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Obstacle|Animation")
	void RisingAnimation();

	/** Triggers the obstacle's falling animation */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Obstacle|Animation")
	void FallingAnimation();
};
