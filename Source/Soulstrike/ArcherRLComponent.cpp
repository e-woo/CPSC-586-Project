#include "ArcherRLComponent.h"
#include "GameFramework/Character.h"

float UArcherRLComponent::CalculateReward()
{
	if (!OwnerCharacter)
		return 0.0f;

	float DistNorm = CurrentState.DistanceToPlayer;
	float PrevActualDistance = PreviousDistanceToPlayer * MaxAttackRange;
	float DeltaDistance = ActualDistanceToPlayer - PrevActualDistance;

	// Reward staying within ~0.95 of max attack range, punish being too close or too far
	bool bInKiteBand = (DistNorm >= 0.9f && DistNorm <= 1.0f && !CurrentState.bIsBeyondMaxRange);
	bool bTooClose = DistNorm < 0.75f;
	bool bTooFar = DistNorm > 1.1f;

	// Component contributions
	float R_PosBand = 0.f;
	float R_MoveAdjust = 0.f;
	float R_AttackTiming = 0.f;
	float R_DPSBase = 0.f;
	float R_DPSDelta = 0.f;
	float R_Survival = 0.f;
	float R_Cover = 0.f;
	float R_LOS = 0.f;
	float R_IdlePenalty = 0.f;

	// Positive reward for being in kiting distance band, else negative
	if (bInKiteBand) R_PosBand += 1.5f;
	else {
		if (bTooClose) R_PosBand -= (0.75f - DistNorm) * 1.0f;
		if (bTooFar) R_PosBand -= (DistNorm - 1.1f) * 0.5f;
	}

	if (bTooClose && DeltaDistance > 0.0f)
		R_MoveAdjust += FMath::Min(0.5f, DeltaDistance / MaxAttackRange * 2.0f);
	if (bTooFar && DeltaDistance < 0.0f)
		R_MoveAdjust += FMath::Min(0.5f, -DeltaDistance / MaxAttackRange * 2.0f);

	if (LastAction == EEliteAction::Primary_Attack)
		R_AttackTiming += (bInKiteBand && CurrentState.bHasLineOfSightToPlayer) ? 1.0f : -0.5f;

	// DPS-based rewards
	float CurrentDPS = GetAverageDPS();
	float DeltaDPS = CurrentDPS - PreviousDPS;
	R_DPSBase += FMath::Clamp(CurrentDPS * 0.5f, 0.0f, 1.0f);
	R_DPSDelta += FMath::Clamp(DeltaDPS * 1.0f, -1.0f, 1.0f);

	float DeltaHealth = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
	R_Survival += DeltaHealth * 2.0f;
	if (CurrentState.bTookDamageRecently && bTooClose)
		R_Survival -= 0.5f;

	if (CurrentState.NumNearbyAllies > 0.2f && CurrentState.DistanceToClosestAlly < DistNorm)
		R_Cover += 0.4f;
	if (bInKiteBand && CurrentState.bHasLineOfSightToPlayer)
		R_LOS += 0.3f;

	if (!bInKiteBand && FMath::Abs(DeltaDistance) < 5.f)
		R_IdlePenalty -= 0.1f;

	float Reward = R_PosBand + R_MoveAdjust + R_AttackTiming + R_DPSBase + R_DPSDelta + R_Survival + R_Cover + R_LOS + R_IdlePenalty;
	LastReward = Reward;

	if (bDebugMode)
	{
		UE_LOG(LogTemp, Log, TEXT("ArcherReward: Pos=%.2f Move=%.2f Attack=%.2f DPS=%.2f dDPS=%.2f Survival=%.2f Cover=%.2f LOS=%.2f Idle=%.2f Total=%.2f Dist=%.2f dDist=%.1f"),
			R_PosBand, R_MoveAdjust, R_AttackTiming, R_DPSBase, R_DPSDelta, R_Survival, R_Cover, R_LOS, R_IdlePenalty, Reward, DistNorm, DeltaDistance);
	}
	return Reward;
}
