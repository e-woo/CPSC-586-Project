#include "AssassinRLComponent.h"
#include "EnemyLogicManager.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

void UAssassinRLComponent::UpdatePoisons(float DeltaTime)
{
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

		// Tick damage
		if (Poison.TimeSinceLastTick >= Poison.TickInterval)
		{
			// Record DPS for RL
			RecordDamageDealt(Poison.DamagePerTick);

			// Report poison damage to Enemy Logic Manager
			if (EnemyLogicMgr && Player)
			{
				EnemyLogicMgr->ReportDamageToPlayer(Player, Poison.DamagePerTick, OwnerCharacter);
				UE_LOG(LogTemp, Log, TEXT("Assassin poison tick: %.0f damage"), Poison.DamagePerTick);
			}
			
			Poison.TimeSinceLastTick = 0.0f;
		}

		// Remove expired poisons
		if (Poison.RemainingDuration <= 0.0f)
		{
			ActivePoisons.RemoveAt(i);
			UE_LOG(LogTemp, Log, TEXT("Assassin poison expired"));
		}
	}
}

void UAssassinRLComponent::OnAttackWindupComplete()
{
	// Call base implementation to handle damage
	Super::OnAttackWindupComplete();

	// Apply poison effect after successful hit
	if (!OwnerCharacter)
		return;

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
		return;

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0);
	if (!Player)
		return;

	// Check if player is in range (same check as base class)
	float CurrentDistanceToPlayer = FVector::Dist(OwnerCharacter->GetActorLocation(), Player->GetActorLocation());
	
	if (CurrentDistanceToPlayer <= MaxAttackRange)
	{
		// Apply poison (90 damage over 3 seconds)
		FPoisonEffect NewPoison;
		NewPoison.RemainingDuration = 3.0f;
		NewPoison.TickInterval = 0.5f;
		NewPoison.DamagePerTick = 90.0f / (3.0f / 0.5f);  // 90 / 6 ticks = 15 per tick
		NewPoison.TimeSinceLastTick = 0.0f;
		
		ActivePoisons.Add(NewPoison);
		
		UE_LOG(LogTemp, Log, TEXT("Assassin: Applied poison (90 damage over 3s)"));
	}
}

float UAssassinRLComponent::CalculateReward()
{
	if (!OwnerCharacter)
		return 0.0f;

	float Reward = 0.0f;

	// Calculate distance delta (using actual distance stored in BuildState)
	float PrevActualDistance = PreviousDistanceToPlayer * MaxAttackRange; // Convert normalized back
	float DeltaDistance = ActualDistanceToPlayer - PrevActualDistance;

	// === DISTANCE MANAGEMENT (Conditional on Poison State) ===
	bool HasPoisonActive = HasActivePoisonOnPlayer();

	if (CurrentState.bIsBeyondMaxRange)
	{
		// OUT OF RANGE: Force engagement
		if (DeltaDistance < 0.0f) {  // Moving closer
			Reward += -DeltaDistance * 0.05f;  // 50.0 per 1000 units = 0.05 per unit
		} else if (DeltaDistance > 0.0f) {  // Moving away
			Reward -= DeltaDistance * 0.05f;
		} else {
			Reward -= 5.0f;
		}
	}
	else if (HasPoisonActive)
	{
		// POISON ACTIVE: Retreat and let it tick (hit-and-run)
		if (DeltaDistance > 0.0f) {  // Moving away = good
			Reward += DeltaDistance * 0.03f;  // 30.0 per 1000 units
		} else if (DeltaDistance < 0.0f) {  // Still closing = bad
			Reward += DeltaDistance * 0.02f;  // Penalty (negative)
		}
	}
	else if (CanAttack())
	{
		// POISON EXPIRED + ATTACK READY: Dive back in
		if (DeltaDistance < 0.0f) {  // Closing gap = good
			Reward += -DeltaDistance * 0.025f;  // 25.0 per 1000 units
		}
	}

	// === DPS REWARDS (High burst damage) ===
	float CurrentDPS = GetAverageDPS();
	float DeltaDPS = CurrentDPS - PreviousDPS;

	// Base reward (highest multiplier for burst damage)
	Reward += CurrentDPS * 3.0f;

	// Delta bonus
	Reward += DeltaDPS * 5.0f;

	// === HEALTH MANAGEMENT ===
	float DeltaHealth = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
	Reward += DeltaHealth * 10.0f;

	// === SMALL STATIC BONUSES ===
	// Mobility (strafing = dodging)
	if (LastAction == EEliteAction::Strafe_Left || LastAction == EEliteAction::Strafe_Right) {
		Reward += 0.5f;
	}

	// Using allies as cover when retreating
	if (HasPoisonActive && CurrentState.NumNearbyAllies > 0.2f &&
		CurrentState.DistanceToClosestAlly < CurrentState.DistanceToPlayer) {
		Reward += 1.0f;
	}

	return Reward;
}
