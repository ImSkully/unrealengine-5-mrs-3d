// Fill out your copyright notice in the Description page of Project Settings.

#include "MotionControllerComponentCustom.h"
#include "GameFramework/Pawn.h"
#include "PrimitiveSceneProxy.h"
#include "Misc/ScopeLock.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "Features/IModularFeatures.h"
#include "IMotionController.h"
#include "PrimitiveSceneInfo.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "IXRSystemAssets.h"
#include "Components/StaticMeshComponent.h"
#include "MotionDelayBuffer.h"
#include "UObject/VRObjectVersion.h"
#include "UObject/UObjectGlobals.h" // for FindObject<>
#include "XRMotionControllerBase.h"
#include "IXRTrackingSystem.h"
#include "IXRCamera.h"
#include "VRPawn.h"
#include "CameraComponentCustom.h"

namespace {
	/** This is to prevent destruction of motion controller components while they are
		in the middle of being accessed by the render thread */
	FCriticalSection CritSect;

	/** Console variable for specifying whether motion controller late update is used */
	TAutoConsoleVariable<int32> CVarEnableMotionControllerLateUpdate(
		TEXT("vr.EnableMotionControllerLateUpdate"),
		1,
		TEXT("This command allows you to specify whether the motion controller late update is applied.\n")
		TEXT(" 0: don't use late update\n")
		TEXT(" 1: use late update (default)"),
		ECVF_Cheat);
} // anonymous namespace

void UMotionControllerComponentCustom::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsActive)
	{
		return;
	}

	FVector Position;
	FRotator Orientation;
	const float WorldToMeters = GetWorld() ? GetWorld()->GetWorldSettings()->WorldToMeters : 100.0f;
	const bool bNewTrackedState = PollControllerState(Position, Orientation, WorldToMeters);
	
	if (bNewTrackedState)
	{
		AVRPawn* VRPawn = Cast<AVRPawn>(GetAttachmentRootActor());
		if (!VRPawn)
		{
			return;
		}

		FTransform WorldTransform;
		{
			FScopeLock Lock(&VRPawn->TransformCrit);
			const FTransform CameraTransform = VRPawn->Camera->GetComponentToWorld();
			
			if (TSharedPtr<IXRTrackingSystem, ESPMode::ThreadSafe> XRSystem = GEngine->XRSystem)
			{
				TSharedPtr<IXRCamera, ESPMode::ThreadSafe> XRCamera = XRSystem->GetXRCamera();
				if (XRCamera.IsValid() && XRSystem->IsHeadTrackingAllowed())
				{
					FQuat HMDOrientation;
					FVector HMDPosition;
					XRCamera->UpdatePlayerCamera(HMDOrientation, HMDPosition);
					
					const FTransform HMDTransform(HMDOrientation, HMDPosition);
					const FTransform ControllerTransform(Orientation, Position);
					WorldTransform = ControllerTransform.GetRelativeTransform(HMDTransform) * CameraTransform;
				}
			}
		}
		
		SetWorldTransform(WorldTransform);
	}

	// Refresh display model when tracking begins
	if (!bTracked && bNewTrackedState && bDisplayDeviceModel && DisplayModelSource != UMotionControllerComponent::CustomModelSourceId)
	{
		RefreshDisplayComponent();
	}
	
	bTracked = bNewTrackedState;

	// Initialize view extension for late updates
	if (!ViewExtension.IsValid() && GEngine)
	{
		ViewExtension = FSceneViewExtensions::NewExtension<FViewExtension>(this);
	}
}

bool UMotionControllerComponentCustom::PollControllerState(FVector& Position, FRotator& Orientation, float WorldToMetersScale)
{
	if (IsInGameThread())
	{
		// Cache state from the game thread for use on the render thread
		const AActor* MyOwner = GetOwner();
		const APawn* MyPawn = Cast<APawn>(MyOwner);
		bHasAuthority = MyPawn ? MyPawn->IsLocallyControlled() : (MyOwner->Role == ENetRole::ROLE_Authority);
	}

	if (bHasAuthority)
	{
		TArray<IMotionController*> MotionControllers = IModularFeatures::Get().GetModularFeatureImplementations<IMotionController>(IMotionController::GetModularFeatureName());
		for (auto MotionController : MotionControllers)
		{
			if (MotionController == nullptr)
			{
				continue;
			}

			CurrentTrackingStatus = MotionController->GetControllerTrackingStatus(PlayerIndex, MotionSource);
			if (MotionController->GetControllerOrientationAndPosition(PlayerIndex, MotionSource, Orientation, Position, WorldToMetersScale))
			{
				if (IsInGameThread())
				{
					InUseMotionController = MotionController;
					OnMotionControllerUpdated();
					InUseMotionController = nullptr;
				}
				return true;
			}
		}

		if (MotionSource == FXRMotionControllerBase::HMDSourceId)
		{
			IXRTrackingSystem* TrackingSys = GEngine->XRSystem.Get();
			if (TrackingSys)
			{
				FQuat OrientationQuat;
				if (TrackingSys->GetCurrentPose(IXRTrackingSystem::HMDDeviceId, OrientationQuat, Position))
				{
					Orientation = OrientationQuat.Rotator();
					return true;
				}
			}
		}
	}
	return false;
}

//=============================================================================
UMotionControllerComponentCustom::FViewExtension::FViewExtension(const FAutoRegister& AutoRegister, UMotionControllerComponentCustom* InMotionControllerComponent)
	: FSceneViewExtensionBase(AutoRegister)
	, MotionControllerComponent(InMotionControllerComponent)
{}

void UMotionControllerComponentCustom::FViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	if (!MotionControllerComponent)
	{
		return;
	}

	// Set up the late update state for the controller component
	LateUpdate.Setup(MotionControllerComponent->CalcNewComponentToWorld(FTransform()), MotionControllerComponent, false);
}

//=============================================================================
void UMotionControllerComponentCustom::FViewExtension::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily)
{
	if (!MotionControllerComponent)
	{
		return;
	}

	FTransform OldTransform;
	FTransform NewTransform;
	{
		FScopeLock ScopeLock(&CritSect);
		if (!MotionControllerComponent)
		{
			return;
		}

		// Find a view that is associated with this player.
		float WorldToMetersScale = -1.0f;
		for (const FSceneView* SceneView : InViewFamily.Views)
		{
			if (SceneView && SceneView->PlayerIndex == MotionControllerComponent->PlayerIndex)
			{
				WorldToMetersScale = SceneView->WorldToMetersScale;
				break;
			}
		}
		// If there are no views associated with this player use view 0.
		if (WorldToMetersScale < 0.0f)
		{
			check(InViewFamily.Views.Num() > 0);
			WorldToMetersScale = InViewFamily.Views[0]->WorldToMetersScale;
		}

		// Poll state for the most recent controller transform
		FVector Position;
		FRotator Orientation;
		if (!MotionControllerComponent->PollControllerState(Position, Orientation, WorldToMetersScale))
		{
			return;
		}

		OldTransform = MotionControllerComponent->RenderThreadRelativeTransform;
		NewTransform = FTransform(Orientation, Position, MotionControllerComponent->RenderThreadComponentScale);
	} // Release the lock on the MotionControllerComponent

	// Tell the late update manager to apply the offset to the scene components
	LateUpdate.Apply_RenderThread(InViewFamily.Scene, OldTransform, NewTransform);
}

void UMotionControllerComponentCustom::FViewExtension::PostRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily)
{
	if (!MotionControllerComponent)
	{
		return;
	}
	LateUpdate.PostRender_RenderThread();
}

bool UMotionControllerComponentCustom::FViewExtension::IsActiveThisFrame(class FViewport* InViewport) const
{
	check(IsInGameThread());
	return MotionControllerComponent && !MotionControllerComponent->bDisableLowLatencyUpdate && CVarEnableMotionControllerLateUpdate.GetValueOnGameThread();
}
