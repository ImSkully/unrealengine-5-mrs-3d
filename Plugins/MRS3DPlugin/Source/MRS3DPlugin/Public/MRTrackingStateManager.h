#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BitmapPoint.h"
#include "PlaneDetection.h"
#include "MRTrackingStateManager.generated.h"

UENUM(BlueprintType)
enum class EMRTrackingQuality : uint8
{
	None        UMETA(DisplayName = "None"),
	Limited     UMETA(DisplayName = "Limited"),
	Normal      UMETA(DisplayName = "Normal"),
	Excellent   UMETA(DisplayName = "Excellent")
};

USTRUCT(BlueprintType)
struct FMRSessionInfo
{
	GENERATED_BODY()

	/** Current tracking state */
	UPROPERTY(BlueprintReadOnly, Category = "Tracking")
	ETrackingState TrackingState;

	/** Current tracking quality */
	UPROPERTY(BlueprintReadOnly, Category = "Tracking")
	EMRTrackingQuality TrackingQuality;

	/** Session start time */
	UPROPERTY(BlueprintReadOnly, Category = "Tracking")
	float SessionStartTime;

	/** Total session duration */
	UPROPERTY(BlueprintReadOnly, Category = "Tracking")
	float SessionDuration;

	/** Number of tracking interruptions */
	UPROPERTY(BlueprintReadOnly, Category = "Tracking")
	int32 TrackingInterruptions;

	/** Last known camera pose */
	UPROPERTY(BlueprintReadOnly, Category = "Tracking")
	FTransform LastKnownPose;

	/** Tracking confidence (0.0 to 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "Tracking")
	float TrackingConfidence;

	FMRSessionInfo()
		: TrackingState(ETrackingState::NotTracking)
		, TrackingQuality(EMRTrackingQuality::None)
		, SessionStartTime(0.0f)
		, SessionDuration(0.0f)
		, TrackingInterruptions(0)
		, LastKnownPose(FTransform::Identity)
		, TrackingConfidence(0.0f)
	{}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTrackingStateChanged, ETrackingState, OldState, ETrackingState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTrackingQualityChanged, EMRTrackingQuality, OldQuality, EMRTrackingQuality, NewQuality);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrackingLost, float, LostDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTrackingRecovered);

/**
 * Manages Mixed Reality tracking state and provides session monitoring
 * Handles tracking state transitions, quality assessment, and recovery
 */
UCLASS(BlueprintType)
class MRS3DPLUGIN_API UMRTrackingStateManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UMRTrackingStateManager();

	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem Interface

	/** Update tracking state and camera pose */
	UFUNCTION(BlueprintCallable, Category = "Tracking")
	void UpdateTrackingState(ETrackingState NewState, const FTransform& CameraPose, float Confidence = 1.0f);

	/** Get current tracking state */
	UFUNCTION(BlueprintPure, Category = "Tracking")
	ETrackingState GetTrackingState() const { return SessionInfo.TrackingState; }

	/** Get current tracking quality */
	UFUNCTION(BlueprintPure, Category = "Tracking")
	EMRTrackingQuality GetTrackingQuality() const { return SessionInfo.TrackingQuality; }

	/** Get complete session information */
	UFUNCTION(BlueprintPure, Category = "Tracking")
	FMRSessionInfo GetSessionInfo() const { return SessionInfo; }

	/** Check if tracking is currently stable */
	UFUNCTION(BlueprintPure, Category = "Tracking")
	bool IsTrackingStable() const;

	/** Check if tracking recovery is recommended */
	UFUNCTION(BlueprintPure, Category = "Tracking")
	bool ShouldAttemptRecovery() const;

	/** Attempt to recover tracking */
	UFUNCTION(BlueprintCallable, Category = "Tracking")
	void AttemptTrackingRecovery();

	/** Reset tracking session statistics */
	UFUNCTION(BlueprintCallable, Category = "Tracking")
	void ResetSession();

	/** Get tracking statistics */
	UFUNCTION(BlueprintCallable, Category = "Tracking")
	void GetTrackingStats(float& AverageConfidence, float& UptimePercentage, int32& TotalInterruptions) const;

	/** Set tracking quality thresholds */
	UFUNCTION(BlueprintCallable, Category = "Configuration")
	void SetQualityThresholds(float ExcellentThreshold = 0.9f, float NormalThreshold = 0.7f, float LimitedThreshold = 0.3f);

	/** Events for tracking state changes */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTrackingStateChanged OnTrackingStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTrackingQualityChanged OnTrackingQualityChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTrackingLost OnTrackingLost;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTrackingRecovered OnTrackingRecovered;

protected:
	/** Current session information */
	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FMRSessionInfo SessionInfo;

	/** Quality threshold for excellent tracking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float ExcellentQualityThreshold;

	/** Quality threshold for normal tracking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float NormalQualityThreshold;

	/** Quality threshold for limited tracking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float LimitedQualityThreshold;

	/** Maximum time to wait before recommending recovery */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float MaxTrackingLossTime;

	/** Minimum stable time before considering tracking recovered */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float MinStableTrackingTime;

private:
	/** Tracking state history for stability analysis */
	struct FTrackingStateEntry
	{
		ETrackingState State;
		float Timestamp;
		float Confidence;
		FTransform Pose;
	};

	/** Recent tracking state history */
	TArray<FTrackingStateEntry> StateHistory;

	/** Maximum entries to keep in history */
	static constexpr int32 MaxHistorySize = 100;

	/** Time when tracking was last lost */
	float TrackingLostTime;

	/** Time when tracking was last recovered */
	float TrackingRecoveredTime;

	/** Total confidence values accumulated */
	float TotalConfidence;

	/** Number of confidence samples */
	int32 ConfidenceSamples;

	/** Total uptime (excluding lost tracking periods) */
	float TotalUptime;

	/** Calculate tracking quality based on confidence */
	EMRTrackingQuality CalculateTrackingQuality(float Confidence) const;

	/** Update session duration and statistics */
	void UpdateSessionStats();

	/** Add entry to tracking history */
	void AddToHistory(ETrackingState State, float Confidence, const FTransform& Pose);

	/** Analyze tracking stability from recent history */
	bool AnalyzeTrackingStability() const;

	/** Handle tracking state transition */
	void HandleStateTransition(ETrackingState OldState, ETrackingState NewState);

	/** Handle tracking quality change */
	void HandleQualityChange(EMRTrackingQuality OldQuality, EMRTrackingQuality NewQuality);
};