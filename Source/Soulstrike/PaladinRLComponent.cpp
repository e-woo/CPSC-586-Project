#include "PaladinRLComponent.h"
#include "GameFramework/Character.h"

float UPaladinRLComponent::CalculateReward()
{
	if (!OwnerCharacter)
		return 0.0f;

	float Reward = 0.0f;

	// Calculate distance delta (using actual distance)
	float PrevActualDistance = PreviousDistanceToPlayer * MaxAttackRange;
	float DeltaDistance = ActualDistanceToPlayer - PrevActualDistance;

	// === DISTANCE MANAGEMENT ===
	if (CurrentState.bIsBeyondMaxRange)
	{
		// OUT OF RANGE: Force engagement
		if (DeltaDistance < 0.0f) {
			Reward += -DeltaDistance * 0.05f;  // 50.0 per 1000 units
		} else if (DeltaDistance > 0.0f) {
			Reward -= DeltaDistance * 0.05f;
		} else {
			Reward -= 5.0f;
		}
	}
	else
	{
		// IN RANGE: Paladin wants to move closer (frontline)
		if (DeltaDistance < 0.0f) {  // Moving closer = good
			Reward += -DeltaDistance * 0.01f;  // 10.0 per 1000 units
		}
	}

	// === DPS REWARDS (Base + Delta Bonus) ===
	float CurrentDPS = GetAverageDPS();
	float DeltaDPS = CurrentDPS - PreviousDPS;

	Reward += CurrentDPS * 1.5f;  // Moderate DPS multiplier
	Reward += DeltaDPS * 5.0f;    // Delta bonus

	// === HEALTH MANAGEMENT ===
	float DeltaHealth = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
	Reward += DeltaHealth * 10.0f;

	// === HEALER PROTECTION (Dominant Feature) ===
	TArray<ACharacter*> ClosestAllies;
	FindClosestAllies(ClosestAllies, 3);

	ACharacter* Healer = nullptr;
	float DistanceToHealer = 9999.0f;
	
	for (ACharacter* Ally : ClosestAllies)
	{
		if (Ally && Ally->IsValidLowLevel() && !Ally->IsPendingKillOrUnreachable() && 
			Ally->GetName().Contains("Healer"))
		{
			Healer = Ally;
			DistanceToHealer = FVector::Dist(OwnerCharacter->GetActorLocation(), Ally->GetActorLocation());
			break;
		}
	}

	if (Healer && Healer->IsValidLowLevel())
	{
		// Reward for being between player and healer (bodyguard position)
		float DistPlayerToHealer = FVector::Dist(CachedPlayerLocation, Healer->GetActorLocation());
		float DistSelfToPlayer = ActualDistanceToPlayer;

		if (DistanceToHealer < DistPlayerToHealer && DistSelfToPlayer < DistPlayerToHealer) {
			Reward += 10.0f;  // Excellent bodyguard position
		} else if (DistanceToHealer < 800.0f) {
			Reward += 3.0f;  // Near healer
		}
	}

	// === FRONTLINE BONUSES ===
	// Team cohesion
	Reward += CurrentState.NumNearbyAllies * 1.0f;

	// Frontline tanking
	if (CurrentState.DistanceToPlayer < 0.4f) {
		Reward += 2.0f;
	}

	// Taking damage (drawing aggro)
	if (CurrentState.bTookDamageRecently) {
		Reward += 1.0f;
	}

	return Reward;
}
