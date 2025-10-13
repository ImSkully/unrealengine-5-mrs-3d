// Copyright (c) MIT License

#include "FirstPersonBase.h"
#include "MyGameInstanceCode.h"

// Sets default values
AFirstPersonBase::AFirstPersonBase()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Default Scene Root"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->bLockToHmd = true;
	Camera->AttachToComponent(DefaultSceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
	Camera->SetRelativeLocation(FVector(0, 0, 170));
}

// Called when the game starts or when spawned
void AFirstPersonBase::BeginPlay()
{
	Super::BeginPlay();
	InitLocation = Camera->GetComponentLocation();
}

// Called every frame
void AFirstPersonBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FScopeLock Lock(&TransformCrit);
	Camera->SetWorldRotation(InternalTransform.Rotator());
	Camera->SetWorldLocation(InternalTransform.GetLocation() + InitLocation);
}

// Called to bind functionality to input
void AFirstPersonBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AFirstPersonBase::TrackBodyIK(sl::Pose& zed_pose, UMyGameInstanceCode* GameInstance)
{
	FTransform Transform = sl::unreal::ToUnrealType(sl::Transform(zed_pose.getRotation(), zed_pose.getTranslation()));

	if (IsValid(this))
	{
		FScopeLock Lock(&TransformCrit);
		InternalTransform = Transform;
	}
	else
	{
		FScopeLock Lock(&GameInstance->InitTransformCrit);
		GameInstance->InitTransform = Transform;
	}
}

void AFirstPersonBase::BeginDestroy()
{
	Super::BeginDestroy();
	std::unique_lock<std::shared_mutex> Lock(ReleaseMemoryLock);
}
