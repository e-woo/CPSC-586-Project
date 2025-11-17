#include "ArcherRLComponent.h"

float UArcherRLComponent::CalculateReward()
{
	if (!OwnerCharacter)
		return 0.0f;

	float Reward = 0.0f;

	// === DISTANCE DELTA (Conditional Scaling) ===
	float DeltaDistance = CurrentState.DistanceToPlayer - PreviousDistanceToPlayer;

	if (CurrentState.bIsBeyondMaxRange)
	{
		// OUT OF RANGE: Force engagement by heavily rewarding approach
		if (DeltaDistance < 0.0f) {  // Moving closer
			Reward += -DeltaDistance * 50.0f;// High reward
		} else if (DeltaDistance > 0.0f) {  // Moving away
			Reward -= DeltaDistance * 50.0f;  // High penalty
		} else {  // No change
			Reward -= 5.0f;  // Penalty for not approaching
		}
	}
	else
	{
		// IN RANGE: Archer wants to maintain/increase distance (kite)
		if (DeltaDistance > 0.0f) {  // Moving away = good
			Reward += DeltaDistance * 20.0f;
		} else if (DeltaDistance < 0.0f) {  // Getting closer = bad
			Reward += DeltaDistance * 15.0f;  // Will be negative
		}
	}

	// === DPS REWARDS (Base + Delta Bonus) ===
	float CurrentDPS = GetAverageDPS();
	float DeltaDPS = CurrentDPS - PreviousDPS;

	// Base reward for dealing damage
	Reward += CurrentDPS * 2.0f;  // Archer DPS multiplier

	// Bonus for increasing DPS
	Reward += DeltaDPS * 5.0f;

	// === HEALTH DELTA ===
	float DeltaHealth = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
	Reward += DeltaHealth * 10.0f;

	// === SMALL STATIC BONUSES ===
	// Line of sight (enables attacks)
	if (CurrentState.bHasLineOfSightToPlayer) {
		Reward += 0.5f;
	}

	// Behind allies (safety)
	if (CurrentState.NumNearbyAllies > 0.2f && 
		CurrentState.DistanceToClosestAlly < CurrentState.DistanceToPlayer) {
		Reward += 1.0f;
	}

	return Reward;
}
