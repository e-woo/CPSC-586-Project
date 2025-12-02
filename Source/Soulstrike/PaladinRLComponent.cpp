#include "PaladinRLComponent.h"
#include "GameFramework/Character.h"

static bool GPaladinVerboseReward = true; // debugging

float UPaladinRLComponent::CalculateReward()
{
	if (!OwnerCharacter)
		return 0.0f;

	float DistNorm = CurrentState.DistanceToPlayer;
	float PrevActualDistance = PreviousDistanceToPlayer * MaxAttackRange;
	float DeltaDistance = ActualDistanceToPlayer - PrevActualDistance;
	bool bInFrontlineBand = (DistNorm >= 0.4f && DistNorm <= 0.8f && !CurrentState.bIsBeyondMaxRange);
	bool bTooFar = DistNorm > 1.1f;
	bool bTooClose = DistNorm < 0.3f;

	float R_Pos=0, R_Move=0, R_Attack=0, R_DPSBase=0, R_DPSDelta=0, R_Survive=0, R_Guard=0, R_Cohesion=0, R_Camping=0;

	// Reward "frontlining" (tanking for other elites), stay close to player
	R_Pos += bInFrontlineBand ? 1.0f : 0.0f;
	if (bTooFar) R_Camping -= (DistNorm - 1.1f) * 0.8f;
	if (bTooClose) R_Pos -= (0.3f - DistNorm) * 0.5f;

	if (!bInFrontlineBand && DistNorm > 0.8f && DeltaDistance < 0.0f) R_Move += FMath::Min(0.4f, -DeltaDistance / MaxAttackRange * 2.0f);
	if (!bInFrontlineBand && bTooClose && DeltaDistance > 0.0f) R_Move += FMath::Min(0.4f, DeltaDistance / MaxAttackRange * 2.0f);

	// Reward melee attacks
	if (LastAction == EEliteAction::Primary_Attack)
	{
		if (!CurrentState.bIsBeyondMaxRange && AttackState == EAttackState::Normal)
			R_Attack += 0.7f; // reward melee swings
		else
			R_Attack -= 0.3f;
	}

	// Reward DPS
	float CurrentDPS = GetAverageDPS();
	float DeltaDPS = CurrentDPS - PreviousDPS;
	R_DPSBase += FMath::Clamp(CurrentDPS * 0.4f, 0.f, 0.8f);
	R_DPSDelta += FMath::Clamp(DeltaDPS * 0.8f, -0.6f, 0.6f);

	// Stay alive
	float DeltaHealth = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
	R_Survive += DeltaHealth * 3.0f;
	if (CurrentState.bTookDamageRecently && bInFrontlineBand) R_Survive += 0.4f;

	// Tnak/protect healer/archer
	TArray<ACharacter*> Allies;
	FindClosestAllies(Allies, 3);
	for (ACharacter* Ally : Allies)
	{
		if (!Ally) continue;
		FString N = Ally->GetName();
		bool bProtectedType = N.Contains("Healer") || N.Contains("Archer");
		if (bProtectedType)
		{
			float DistToAlly = FVector::Dist(OwnerCharacter->GetActorLocation(), Ally->GetActorLocation());
			float PlayerToAlly = FVector::Dist(CachedPlayerLocation, Ally->GetActorLocation());
			if (DistToAlly <= 700.0f && DistNorm * MaxAttackRange < PlayerToAlly) R_Guard += 0.7f;
		}
	}

	R_Cohesion += CurrentState.NumNearbyAllies * 0.2f;

	float Reward = R_Pos + R_Move + R_Attack + R_DPSBase + R_DPSDelta + R_Survive + R_Guard + R_Cohesion + R_Camping;
	LastReward = Reward;

	if (bDebugMode && GPaladinVerboseReward)
	{
		UE_LOG(LogTemp, Verbose, TEXT("PaladinRewardComponents: Pos=%.2f Move=%.2f Attack=%.2f DPS=%.2f dDPS=%.2f Survive=%.2f Guard=%.2f Cohesion=%.2f Camp=%.2f Total=%.2f Dist=%.2f dDist=%.1f"),
			R_Pos, R_Move, R_Attack, R_DPSBase, R_DPSDelta, R_Survive, R_Guard, R_Cohesion, R_Camping, Reward, DistNorm, DeltaDistance);
	}
	return Reward;
}
