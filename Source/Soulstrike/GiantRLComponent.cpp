#include "GiantRLComponent.h"

float UGiantRLComponent::CalculateReward()
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
		// IN RANGE: Giant wants to stay close (tank)
		if (DeltaDistance < 0.0f) {  // Moving closer = good
			Reward += -DeltaDistance * 15.0f;
		} else if (DeltaDistance > 0.0f) {  // Retreating = bad
			Reward -= DeltaDistance * 10.0f;
		}
	}

	// === DPS REWARDS (Base + Delta Bonus) ===
	float CurrentDPS = GetAverageDPS();
	float DeltaDPS = CurrentDPS - PreviousDPS;

	Reward += CurrentDPS * 2.0f;  // Base DPS
	Reward += DeltaDPS * 5.0f;    // Delta bonus

	// === HEALTH DELTA (High priority for tanks) ===
	float DeltaHealth = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
	Reward += DeltaHealth * 15.0f;  // Higher multiplier for tank survival

	// === STATIC BONUSES ===
	// Survival (small static bonus)
	Reward += CurrentState.SelfHealthPercentage * 1.0f;

	// Frontline position
	if (CurrentState.DistanceToPlayer < 0.3f) {
		Reward += 3.0f;
	}

	// Taking damage (tanking for team)
	if (CurrentState.bTookDamageRecently) {
		Reward += 2.0f;
	}

	// Blocking line of sight
	if (CurrentState.bHasLineOfSightToPlayer) {
		Reward += 1.0f;
	}

	// Having allies behind (protecting them)
	if (CurrentState.NumNearbyAllies > 0.2f &&
		CurrentState.DistanceToClosestAlly > CurrentState.DistanceToPlayer) {
		Reward += 2.0f;
	}

	return Reward;
}
