#include "MRTrackingStateManager.h"
#include "Engine/Engine.h"

UMRTrackingStateManager::UMRTrackingStateManager()
	: ExcellentQualityThreshold(0.9f)
	, NormalQualityThreshold(0.7f)
	, LimitedQualityThreshold(0.3f)
	, MaxTrackingLossTime(30.0f)
	, MinStableTrackingTime(5.0f)
	, TrackingLostTime(0.0f)
	, TrackingRecoveredTime(0.0f)
	, TotalConfidence(0.0f)
	, ConfidenceSamples(0)
	, TotalUptime(0.0f)
{
	StateHistory.Reserve(MaxHistorySize);
}

void UMRTrackingStateManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	ResetSession();
	
	UE_LOG(LogTemp, Log, TEXT("MR Tracking State Manager initialized"));
}

void UMRTrackingStateManager::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("MR Tracking State Manager deinitialized"));
	
	Super::Deinitialize();
}

void UMRTrackingStateManager::UpdateTrackingState(ETrackingState NewState, const FTransform& CameraPose, float Confidence)
{
	const ETrackingState OldState = SessionInfo.TrackingState;
	const EMRTrackingQuality OldQuality = SessionInfo.TrackingQuality;
	
	// Update session info
	SessionInfo.TrackingState = NewState;
	SessionInfo.LastKnownPose = CameraPose;
	SessionInfo.TrackingConfidence = FMath::Clamp(Confidence, 0.0f, 1.0f);
	
	// Calculate new quality
	const EMRTrackingQuality NewQuality = CalculateTrackingQuality(SessionInfo.TrackingConfidence);
	SessionInfo.TrackingQuality = NewQuality;
	
	// Add to history
	AddToHistory(NewState, SessionInfo.TrackingConfidence, CameraPose);
	
	// Update statistics
	UpdateSessionStats();
	
	// Handle state transitions
	if (OldState != NewState)
	{
		HandleStateTransition(OldState, NewState);
	}
	
	// Handle quality changes
	if (OldQuality != NewQuality)
	{
		HandleQualityChange(OldQuality, NewQuality);
	}
	
	// Update confidence statistics
	TotalConfidence += SessionInfo.TrackingConfidence;
	ConfidenceSamples++;
}

bool UMRTrackingStateManager::IsTrackingStable() const
{
	if (SessionInfo.TrackingState == ETrackingState::NotTracking || SessionInfo.TrackingState == ETrackingState::TrackingLost)
	{
		return false;
	}
	
	// Check if we have enough history
	if (StateHistory.Num() < 10)
	{
		return false;
	}
	
	return AnalyzeTrackingStability();
}

bool UMRTrackingStateManager::ShouldAttemptRecovery() const
{
	if (SessionInfo.TrackingState != ETrackingState::NotTracking && SessionInfo.TrackingState != ETrackingState::TrackingLost)
	{
		return false;
	}
	
	const float CurrentTime = FPlatformTime::Seconds();
	const float LostDuration = CurrentTime - TrackingLostTime;
	
	return LostDuration > MaxTrackingLossTime;
}

void UMRTrackingStateManager::AttemptTrackingRecovery()
{
	UE_LOG(LogTemp, Warning, TEXT("Attempting tracking recovery..."));
	
	// Clear recent history to start fresh
	StateHistory.Empty();
	
	// Reset recovery time
	TrackingRecoveredTime = FPlatformTime::Seconds();
	
	// Note: Actual recovery implementation would depend on the specific MR SDK
	// This is a placeholder for the recovery mechanism
}

void UMRTrackingStateManager::ResetSession()
{
	const float CurrentTime = FPlatformTime::Seconds();
	
	SessionInfo.SessionStartTime = CurrentTime;
	SessionInfo.SessionDuration = 0.0f;
	SessionInfo.TrackingInterruptions = 0;
	SessionInfo.TrackingState = ETrackingState::NotTracking;
	SessionInfo.TrackingQuality = EMRTrackingQuality::None;
	SessionInfo.LastKnownPose = FTransform::Identity;
	SessionInfo.TrackingConfidence = 0.0f;
	
	StateHistory.Empty();
	TrackingLostTime = 0.0f;
	TrackingRecoveredTime = CurrentTime;
	TotalConfidence = 0.0f;
	ConfidenceSamples = 0;
	TotalUptime = 0.0f;
	
	UE_LOG(LogTemp, Log, TEXT("Tracking session reset"));
}

void UMRTrackingStateManager::GetTrackingStats(float& AverageConfidence, float& UptimePercentage, int32& TotalInterruptions) const
{
	AverageConfidence = ConfidenceSamples > 0 ? TotalConfidence / ConfidenceSamples : 0.0f;
	UptimePercentage = SessionInfo.SessionDuration > 0.0f ? (TotalUptime / SessionInfo.SessionDuration) * 100.0f : 0.0f;
	TotalInterruptions = SessionInfo.TrackingInterruptions;
}

void UMRTrackingStateManager::SetQualityThresholds(float ExcellentThreshold, float NormalThreshold, float LimitedThreshold)
{
	ExcellentQualityThreshold = FMath::Clamp(ExcellentThreshold, 0.0f, 1.0f);
	NormalQualityThreshold = FMath::Clamp(NormalThreshold, 0.0f, 1.0f);
	LimitedQualityThreshold = FMath::Clamp(LimitedThreshold, 0.0f, 1.0f);
	
	// Ensure thresholds are in descending order
	if (NormalQualityThreshold >= ExcellentQualityThreshold)
	{
		NormalQualityThreshold = ExcellentQualityThreshold - 0.1f;
	}
	
	if (LimitedQualityThreshold >= NormalQualityThreshold)
	{
		LimitedQualityThreshold = NormalQualityThreshold - 0.1f;
	}
	
	UE_LOG(LogTemp, Log, TEXT("Quality thresholds updated: Excellent=%.2f, Normal=%.2f, Limited=%.2f"),
		ExcellentQualityThreshold, NormalQualityThreshold, LimitedQualityThreshold);
}

EMRTrackingQuality UMRTrackingStateManager::CalculateTrackingQuality(float Confidence) const
{
	if (Confidence >= ExcellentQualityThreshold)
	{
		return EMRTrackingQuality::Excellent;
	}
	else if (Confidence >= NormalQualityThreshold)
	{
		return EMRTrackingQuality::Normal;
	}
	else if (Confidence >= LimitedQualityThreshold)
	{
		return EMRTrackingQuality::Limited;
	}
	else
	{
		return EMRTrackingQuality::None;
	}
}

void UMRTrackingStateManager::UpdateSessionStats()
{
	const float CurrentTime = FPlatformTime::Seconds();
	SessionInfo.SessionDuration = CurrentTime - SessionInfo.SessionStartTime;
	
	// Update uptime if currently tracking
	if (SessionInfo.TrackingState == ETrackingState::FullTracking || SessionInfo.TrackingState == ETrackingState::LimitedTracking)
	{
		static float LastUptimeUpdate = CurrentTime;
		TotalUptime += CurrentTime - LastUptimeUpdate;
		LastUptimeUpdate = CurrentTime;
	}
}

void UMRTrackingStateManager::AddToHistory(ETrackingState State, float Confidence, const FTransform& Pose)
{
	FTrackingStateEntry Entry;
	Entry.State = State;
	Entry.Timestamp = FPlatformTime::Seconds();
	Entry.Confidence = Confidence;
	Entry.Pose = Pose;
	
	StateHistory.Add(Entry);
	
	// Maintain maximum history size
	if (StateHistory.Num() > MaxHistorySize)
	{
		StateHistory.RemoveAt(0);
	}
}

bool UMRTrackingStateManager::AnalyzeTrackingStability() const
{
	if (StateHistory.Num() < 10)
	{
		return false;
	}
	
	const float CurrentTime = FPlatformTime::Seconds();
	const float StabilityWindow = MinStableTrackingTime;
	
	// Check recent tracking states
	int32 StableCount = 0;
	int32 TotalRecentCount = 0;
	
	for (int32 i = StateHistory.Num() - 1; i >= 0; i--)
	{
		const FTrackingStateEntry& Entry = StateHistory[i];
		
		if (CurrentTime - Entry.Timestamp > StabilityWindow)
		{
			break;
		}
		
		TotalRecentCount++;
		
		if ((Entry.State == ETrackingState::FullTracking || Entry.State == ETrackingState::LimitedTracking) && Entry.Confidence >= NormalQualityThreshold)
		{
			StableCount++;
		}
	}
	
	// Consider stable if 80% of recent states are good tracking
	const float StabilityRatio = TotalRecentCount > 0 ? static_cast<float>(StableCount) / TotalRecentCount : 0.0f;
	return StabilityRatio >= 0.8f;
}

void UMRTrackingStateManager::HandleStateTransition(ETrackingState OldState, ETrackingState NewState)
{
	const float CurrentTime = FPlatformTime::Seconds();
	
	// Handle transition to lost tracking
	if ((OldState == ETrackingState::FullTracking || OldState == ETrackingState::LimitedTracking) && 
		(NewState == ETrackingState::TrackingLost || NewState == ETrackingState::NotTracking))
	{
		TrackingLostTime = CurrentTime;
		SessionInfo.TrackingInterruptions++;
		
		const float LostDuration = CurrentTime - TrackingRecoveredTime;
		OnTrackingLost.Broadcast(LostDuration);
		
		UE_LOG(LogTemp, Warning, TEXT("Tracking lost after %.1f seconds"), LostDuration);
	}
	
	// Handle transition to recovered tracking
	else if ((OldState == ETrackingState::TrackingLost || OldState == ETrackingState::NotTracking) && 
			 (NewState == ETrackingState::FullTracking || NewState == ETrackingState::LimitedTracking))
	{
		TrackingRecoveredTime = CurrentTime;
		OnTrackingRecovered.Broadcast();
		
		const float LostDuration = CurrentTime - TrackingLostTime;
		UE_LOG(LogTemp, Log, TEXT("Tracking recovered after %.1f seconds"), LostDuration);
	}
	
	// Broadcast state change event
	OnTrackingStateChanged.Broadcast(OldState, NewState);
}

void UMRTrackingStateManager::HandleQualityChange(EMRTrackingQuality OldQuality, EMRTrackingQuality NewQuality)
{
	OnTrackingQualityChanged.Broadcast(OldQuality, NewQuality);
	
	UE_LOG(LogTemp, Verbose, TEXT("Tracking quality changed from %d to %d"), 
		static_cast<int32>(OldQuality), static_cast<int32>(NewQuality));
}