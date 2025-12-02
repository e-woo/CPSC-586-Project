#include "HealerRLComponent.h"
#include "GameFramework/Character.h"

static bool GHealerVerboseReward = true; // debugging

void UHealerRLComponent::PerformSecondaryAttackOnElite()
{
	Super::PerformSecondaryAttackOnElite(); // heal
}

float UHealerRLComponent::CalculateReward()
{
	if (!OwnerCharacter) return 0.0f;
	float DistNorm = CurrentState.DistanceToPlayer;
	float PrevActualDistance = PreviousDistanceToPlayer * MaxAttackRange;
	float DeltaDistance = ActualDistanceToPlayer - PrevActualDistance;
	bool bSafeBand = DistNorm >= 0.85f;
	bool bTooClose = DistNorm < 0.6f;
	bool bDanger = DistNorm < 0.4f;

	float R_Pos=0,R_Move=0,R_HealBase=0,R_HealDelta=0,R_AttackPenalty=0,R_Survive=0,R_AllyNeed=0,R_Cover=0,R_DPSNeg=0;

	if (bSafeBand) R_Pos += 0.8f;
	if (bTooClose) R_Pos -= (0.6f - DistNorm) * 1.0f;
	if (bDanger) R_Pos -= 0.8f;

	// Stay "safe" (away from player) but prio healing
	if (bTooClose && DeltaDistance > 0.0f) R_Move += FMath::Min(0.5f, DeltaDistance / MaxAttackRange * 2.0f);
	if (bSafeBand && DeltaDistance > 0.0f && CurrentState.DistanceToClosestAlly > DistNorm) R_Move -= 0.2f; // drifting away from allies unnecessarily

	// Reward healing
	float CurrentHPS = GetAverageHPS(); float DeltaHPS = CurrentHPS - PreviousHPS;
	R_HealBase += FMath::Clamp(CurrentHPS * 1.2f, 0.f, 2.0f);
	R_HealDelta += FMath::Clamp(DeltaHPS * 2.0f, -1.5f, 1.5f);

	// Don't want healer to attack often
	float CurrentDPS = GetAverageDPS();
	R_DPSNeg -= FMath::Clamp(CurrentDPS * 0.3f, 0.f, 1.0f);
	if (LastAction == EEliteAction::Primary_Attack) R_AttackPenalty -= 0.5f;
	if (LastAction == EEliteAction::Secondary_Attack && !bTooClose) R_HealBase += 0.6f;

	// Stay alive
	float DeltaHealth = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
	R_Survive += DeltaHealth * 3.0f;
	if (CurrentState.bTookDamageRecently) R_Survive -= 0.4f;

	// Ally survival
	float AvgAllyHealth = (CurrentState.HealthOfClosestAlly + CurrentState.HealthOfSecondClosestAlly + CurrentState.HealthOfThirdClosestAlly) / 3.0f;
	if (AvgAllyHealth < 0.7f && CurrentState.NumNearbyAllies > 0.0f) R_AllyNeed += 0.6f;
	if (CurrentState.NumNearbyAllies > 0.0f && CurrentState.DistanceToClosestAlly < DistNorm) R_Cover += 0.4f;

	float Reward = R_Pos + R_Move + R_HealBase + R_HealDelta + R_AttackPenalty + R_Survive + R_AllyNeed + R_Cover + R_DPSNeg;
	LastReward = Reward;

	if (bDebugMode && GHealerVerboseReward)
	{
		UE_LOG(LogTemp, Verbose, TEXT("HealerRewardComponents: Pos=%.2f Move=%.2f Heal=%.2f dHeal=%.2f AtkPen=%.2f Survive=%.2f AllyNeed=%.2f Cover=%.2f DPSNeg=%.2f Total=%.2f Dist=%.2f dDist=%.1f"),
			R_Pos,R_Move,R_HealBase,R_HealDelta,R_AttackPenalty,R_Survive,R_AllyNeed,R_Cover,R_DPSNeg,Reward,DistNorm,DeltaDistance);
	}
	return Reward;
}
