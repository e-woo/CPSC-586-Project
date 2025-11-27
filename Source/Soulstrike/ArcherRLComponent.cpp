#include "ArcherRLComponent.h"

float UArcherRLComponent::CalculateReward()
{
	if (!OwnerCharacter)
		return 0.0f;

	float Reward = 0.0f;

	// Calculate distance delta (using actual distance)
	float PrevActualDistance = PreviousDistanceToPlayer * MaxAttackRange;
	float DeltaDistance = ActualDistanceToPlayer - PrevActualDistance;

	// === DISTANCE MANAGEMENT (Conditional Scaling) ===
	if (CurrentState.bIsBeyondMaxRange)
	{
		// OUT OF RANGE: Force engagement by heavily rewarding approach
		if (DeltaDistance < 0.0f) {  // Moving closer
			Reward += -DeltaDistance * 0.05f;  // 50.0 per 1000 units
		} else if (DeltaDistance > 0.0f) {  // Moving away
			Reward -= DeltaDistance * 0.05f;
		} else {  // No change
			Reward -= 5.0f;
		}
	}
	else
	{
		// IN RANGE: Archer wants to maintain/increase distance (kite)
		if (DeltaDistance > 0.0f) {  // Moving away = good
			Reward += DeltaDistance * 0.02f;  // 20.0 per 1000 units
		} else if (DeltaDistance < 0.0f) {  // Getting closer = bad
			Reward += DeltaDistance * 0.015f;  // Will be negative (15.0 penalty per 1000 units)
		}
	}

	// === DPS REWARDS (Base + Delta Bonus) ===
	float CurrentDPS = GetAverageDPS();
	float DeltaDPS = CurrentDPS - PreviousDPS;

	// Base reward for dealing damage
	Reward += CurrentDPS * 2.0f;  // Archer DPS multiplier

	// Bonus for increasing DPS
	Reward += DeltaDPS * 5.0f;

	// === HEALTH MANAGEMENT ===
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
