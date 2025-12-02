#include "AssassinRLComponent.h"
#include "EnemyLogicManager.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

static bool GAssassinVerboseReward = true;

void UAssassinRLComponent::UpdatePoisons(float DeltaTime)
{
	Super::UpdatePoisons(DeltaTime);

	if (!OwnerCharacter)
		return;

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
		return;

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0);
	AEnemyLogicManager* EnemyLogicMgr = Cast<AEnemyLogicManager>(
		UGameplayStatics::GetActorOfClass(World, AEnemyLogicManager::StaticClass())
	);

	for (int32 i = ActivePoisons.Num() - 1; i >= 0; --i)
	{
		FPoisonEffect& Poison = ActivePoisons[i];
		
		Poison.TimeSinceLastTick += DeltaTime;
		Poison.RemainingDuration -= DeltaTime;

		if (Poison.TimeSinceLastTick >= Poison.TickInterval)
		{
			RecordDamageDealt(Poison.DamagePerTick);
			if (EnemyLogicMgr && Player)
			{
				EnemyLogicMgr->ReportDamageToPlayer(Player, Poison.DamagePerTick, OwnerCharacter);
			}
			Poison.TimeSinceLastTick = 0.0f;
		}

		if (Poison.RemainingDuration <= 0.0f)
		{
			ActivePoisons.RemoveAt(i);
		}
	}
}

void UAssassinRLComponent::OnAttackWindupComplete()
{
	Super::OnAttackWindupComplete();

	if (!OwnerCharacter)
		return;

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
		return;

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0);
	if (!Player)
		return;

	float CurrentDistanceToPlayer = FVector::Dist(OwnerCharacter->GetActorLocation(), Player->GetActorLocation());
	
	if (CurrentDistanceToPlayer <= MaxAttackRange)
	{
		FPoisonEffect NewPoison;
		NewPoison.RemainingDuration = 3.0f;
		NewPoison.TickInterval = 0.5f;
		NewPoison.DamagePerTick = 15.0f; // 90 total over duration
		ActivePoisons.Add(NewPoison);
	}
}

float UAssassinRLComponent::CalculateReward()
{
	if (!OwnerCharacter)
		return 0.0f;

	float DistNorm = CurrentState.DistanceToPlayer;
	float PrevActualDistance = PreviousDistanceToPlayer * MaxAttackRange;
	float DeltaDistance = ActualDistanceToPlayer - PrevActualDistance;

	bool bPoisonActive = HasActivePoisonOnPlayer();
	bool bDiveBand = DistNorm <= 0.7f && !CurrentState.bIsBeyondMaxRange; // Go in for attack
	bool bRetreatBand = DistNorm >= 0.9f && DistNorm <= 1.2f; // Retreat while debuff active
	bool bTooFar = DistNorm > 1.3f;

	float R_Dive=0,R_Retreat=0,R_Move=0,R_Attack=0,R_DPSBase=0,R_DPSDelta=0,R_Poison=0,R_Survive=0,R_Strafe=0,R_Camp=0;

	if (bPoisonActive)
		R_Retreat += bRetreatBand ? 1.0f : 0.f;
	else
		R_Dive += bDiveBand ? 0.8f : 0.f;

	if (!bPoisonActive && !bDiveBand && DeltaDistance < 0.0f) R_Move += FMath::Min(0.4f, -DeltaDistance / MaxAttackRange * 2.0f);
	if (bPoisonActive && DistNorm < 0.8f && DeltaDistance > 0.0f) R_Move += FMath::Min(0.4f, DeltaDistance / MaxAttackRange * 2.0f);

	// Reward attacking when debuff not active
	if (LastAction == EEliteAction::Primary_Attack)
	{
		if (!bPoisonActive && bDiveBand && AttackState == EAttackState::Normal) R_Attack += 0.9f; else R_Attack -= 0.4f;
	}

	// Reward DPS
	float CurrentDPS = GetAverageDPS(); float DeltaDPS = CurrentDPS - PreviousDPS;
	R_DPSBase += FMath::Clamp(CurrentDPS * 0.6f, 0.f, 1.2f);
	R_DPSDelta += FMath::Clamp(DeltaDPS * 1.2f, -1.0f, 1.0f);
	R_Poison += ActivePoisons.Num() * 0.3f;

	// Survival reward
	float DeltaHealth = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
	R_Survive += DeltaHealth * 2.0f;
	if (CurrentState.bTookDamageRecently && !bPoisonActive && !bDiveBand) R_Survive -= 0.3f;

	if (bDiveBand && (LastAction == EEliteAction::Strafe_Left || LastAction == EEliteAction::Strafe_Right)) R_Strafe += 0.3f;
	if (bTooFar) R_Camp -= (DistNorm - 1.3f) * 0.8f;

	float Reward = R_Dive + R_Retreat + R_Move + R_Attack + R_DPSBase + R_DPSDelta + R_Poison + R_Survive + R_Strafe + R_Camp;
	LastReward = Reward;
	if (bDebugMode && GAssassinVerboseReward)
	{
		UE_LOG(LogTemp, Verbose, TEXT("AssassinRewardComponents: Dive=%.2f Retreat=%.2f Move=%.2f Attack=%.2f DPS=%.2f dDPS=%.2f Poison=%.2f Survival=%.2f Strafe=%.2f Camp=%.2f Total=%.2f Dist=%.2f dDist=%.1f"),
			R_Dive,R_Retreat,R_Move,R_Attack,R_DPSBase,R_DPSDelta,R_Poison,R_Survive,R_Strafe,R_Camp,Reward,DistNorm,DeltaDistance);
	}
	return Reward;
}
