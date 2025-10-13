// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MotionControllerComponent.h"
#include "MotionControllerComponentCustom.generated.h"

/**
 * Custom Motion Controller Component with enhanced late update support
 */
UCLASS(ClassGroup = (MotionController), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UMotionControllerComponentCustom : public UMotionControllerComponent
{
	GENERATED_BODY()

public:
	UMotionControllerComponentCustom(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** View extension object that persists on the render thread for late updates */
	class FViewExtension : public FSceneViewExtensionBase
	{
	public:
		explicit FViewExtension(const FAutoRegister& AutoRegister, UMotionControllerComponentCustom* InMotionControllerComponent);
		virtual ~FViewExtension() override = default;

		/** ISceneViewExtension interface */
		virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
		virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
		virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
		virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override {}
		virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
		virtual void PostRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
		virtual int32 GetPriority() const override { return -10; }
		virtual bool IsActiveThisFrame(const FViewport* InViewport) const override;

	private:
		friend class UMotionControllerComponentCustom;

		/** Motion controller component associated with this view extension */
		TWeakObjectPtr<UMotionControllerComponentCustom> MotionControllerComponent;
		
		/** Manager for handling late frame updates */
		FLateUpdateManager LateUpdate;
	};

	/** Thread-safe shared pointer to view extension */
	TSharedPtr<FViewExtension, ESPMode::ThreadSafe> ViewExtension;

	/** 
	 * Polls the current state of the motion controller
	 * @param Position Output parameter for controller position
	 * @param Orientation Output parameter for controller rotation
	 * @param WorldToMetersScale Scale factor for converting to world units
	 * @return true if controller state was successfully retrieved
	 */
	bool PollControllerState(FVector& Position, FRotator& Orientation, float WorldToMetersScale);
};
