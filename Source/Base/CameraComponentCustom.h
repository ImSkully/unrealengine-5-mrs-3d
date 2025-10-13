// Copyright (c) MIT License

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "CameraComponentCustom.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_API UCameraComponentCustom : public UCameraComponent
{
	GENERATED_BODY()

public:

	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;

private:

	bool SetUpTransforms = true;
	FTransform Ref = FTransform();
};
