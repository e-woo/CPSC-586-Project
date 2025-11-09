#include "PaladinRLComponent.h"
#include "EliteHealer.h"

float UPaladinRLComponent::CalculateReward()
{
	if (!OwnerElite)
		return 0.0f;

	float Reward = 0.0f;

	// === DISTANCE DELTA ===
	float DeltaDistance = CurrentState.DistanceToPlayer - PreviousDistanceToPlayer;

	if (CurrentState.bIsBeyondMaxRange)
	{
		// OUT OF RANGE: Force engagement
		if (DeltaDistance < 0.0f) {
			Reward += -DeltaDistance * 50.0f;
		} else if (DeltaDistance > 0.0f) {
			Reward -= DeltaDistance * 50.0f;
		} else {
			Reward -= 5.0f;
		}
	}
	else
	{
		// IN RANGE: Paladin wants to move closer (frontline)
		if (DeltaDistance < 0.0f) {  // Moving closer = good
			Reward += -DeltaDistance * 10.0f;
		}
	}

	// === DPS REWARDS (Base + Delta Bonus) ===
	float CurrentDPS = GetAverageDPS();
	float DeltaDPS = CurrentDPS - PreviousDPS;

	Reward += CurrentDPS * 1.5f;  // Base DPS reward
	Reward += DeltaDPS * 5.0f;    // Delta bonus

	// === HEALTH DELTA ===
	float DeltaHealth = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
	Reward += DeltaHealth * 10.0f;

	// === HEALER PROTECTION (Static) ===
	TArray<AEliteEnemy*> ClosestAllies;
	FindClosestAllies(ClosestAllies, 3);

	AEliteHealer* Healer = nullptr;
	float DistanceToHealer = 9999.0f;
	
	for (AEliteEnemy* Ally : ClosestAllies)
	{
		if (Cast<AEliteHealer>(Ally))
		{
			Healer = Cast<AEliteHealer>(Ally);
			DistanceToHealer = FVector::Dist(OwnerElite->GetActorLocation(), Ally->GetActorLocation());
			break;
		}
	}

	if (Healer)
	{
		// Reward for being between player and healer
		float DistPlayerToHealer = FVector::Dist(CachedPlayerLocation, Healer->GetActorLocation());
		float DistSelfToPlayer = FVector::Dist(OwnerElite->GetActorLocation(), CachedPlayerLocation);

		if (DistanceToHealer < DistPlayerToHealer && DistSelfToPlayer < DistPlayerToHealer) {
			Reward += 10.0f;  // Excellent bodyguard position
		} else if (DistanceToHealer < 800.0f) {
			Reward += 3.0f;  // Near healer
		}
	}

	// === SMALL STATIC BONUSES ===
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
