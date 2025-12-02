#include "GiantRLComponent.h"
#include "GameFramework/Character.h"

static bool GGiantVerboseReward = true; // debugging

float UGiantRLComponent::CalculateReward()
{
	if (!OwnerCharacter)
		return 0.0f;

	float DistNorm = CurrentState.DistanceToPlayer;
	float PrevActualDistance = PreviousDistanceToPlayer * MaxAttackRange;
	float DeltaDistance = ActualDistanceToPlayer - PrevActualDistance;
	bool bInTankBand = DistNorm <= 0.6f && !CurrentState.bIsBeyondMaxRange;
	bool bFar = DistNorm > 0.9f;

	float R_Pos = 0.f;
	float R_Move = 0.f;
	float R_Attack = 0.f;
	float R_DPSBase = 0.f;
	float R_DPSDelta = 0.f;
	float R_Tank = 0.f;
	float R_Block = 0.f;
	float R_Cohesion = 0.f;
	float R_Camping = 0.f;

	// Reward being in "tanking" position
	R_Pos += bInTankBand ? 1.0f : 0.0f;
	if (bFar) R_Camping -= (DistNorm - 0.9f) * 1.0f;

	// Movement toward player when far
	if (bFar && DeltaDistance < 0.0f) R_Move += FMath::Min(0.5f, -DeltaDistance / MaxAttackRange * 2.0f);

	// Reward attacking when in range
	if (LastAction == EEliteAction::Primary_Attack)
	{
		if (!CurrentState.bIsBeyondMaxRange && AttackState == EAttackState::Normal)
			R_Attack += 0.8f; // encourage frequent melee attacks
		else
			R_Attack -= 0.3f;
	}

	// DPS rewards
	float CurrentDPS = GetAverageDPS();
	float DeltaDPS = CurrentDPS - PreviousDPS;
	R_DPSBase += FMath::Clamp(CurrentDPS * 0.3f, 0.f, 0.6f);
	R_DPSDelta += FMath::Clamp(DeltaDPS * 0.8f, -0.6f, 0.6f);

	float DeltaHealth = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
	R_Tank += DeltaHealth * 3.0f;
	if (CurrentState.bTookDamageRecently && bInTankBand) R_Tank += 0.5f;

	// Body blocking/Tanking for archer/healer
	TArray<ACharacter*> Allies;
	FindClosestAllies(Allies, 3);
	for (ACharacter* Ally : Allies)
	{
		if (!Ally) continue;
		FString N = Ally->GetName();
		bool bProtectedType = N.Contains("Archer") || N.Contains("Healer");
		if (bProtectedType)
		{
			float DistToAlly = FVector::Dist(OwnerCharacter->GetActorLocation(), Ally->GetActorLocation());
			float PlayerToAlly = FVector::Dist(CachedPlayerLocation, Ally->GetActorLocation());
			if (DistToAlly < PlayerToAlly && DistNorm < PlayerToAlly / MaxAttackRange)
			{
				R_Block += 0.6f;
			}
		}
	}

	R_Cohesion += CurrentState.NumNearbyAllies * (bInTankBand ? 0.3f : 0.1f);

	float Reward = R_Pos + R_Move + R_Attack + R_DPSBase + R_DPSDelta + R_Tank + R_Block + R_Cohesion + R_Camping;
	LastReward = Reward;

	if (bDebugMode && GGiantVerboseReward)
	{
		UE_LOG(LogTemp, Verbose, TEXT("GiantRewardComponents: Pos=%.2f Move=%.2f Attack=%.2f DPS=%.2f dDPS=%.2f Tank=%.2f Block=%.2f Cohesion=%.2f Camp=%.2f Total=%.2f Dist=%.2f dDist=%.1f"),
			R_Pos, R_Move, R_Attack, R_DPSBase, R_DPSDelta, R_Tank, R_Block, R_Cohesion, R_Camping, Reward, DistNorm, DeltaDistance);
	}
	return Reward;
}
