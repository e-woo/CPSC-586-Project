#include "HealerRLComponent.h"

float UHealerRLComponent::CalculateReward()
{
	if (!OwnerElite)
		return 0.0f;

	float Reward = 0.0f;

	// === DISTANCE DELTA ===
	float DeltaDistance = CurrentState.DistanceToPlayer - PreviousDistanceToPlayer;

	if (CurrentState.bIsBeyondMaxRange)
	{
		// SPECIAL: Healer only penalized if ALSO out of range of allies
		bool bInRangeOfAlly = (CurrentState.DistanceToClosestAlly < 0.5f);

		if (!bInRangeOfAlly)  // Out of range of both player AND allies = bad
		{
			if (DeltaDistance < 0.0f) {
				Reward += -DeltaDistance * 50.0f;  // Move toward someone
			} else if (DeltaDistance > 0.0f) {
				Reward -= DeltaDistance * 50.0f;
			} else {
				Reward -= 5.0f;
			}
		}
	}
	else
	{
		// IN RANGE: Healer wants to increase distance (stay safe)
		if (DeltaDistance > 0.0f) {  // Moving away = good
			Reward += DeltaDistance * 20.0f;
		} else if (DeltaDistance < 0.0f) {  // Getting closer = bad
			Reward -= -DeltaDistance * 15.0f;
		}
	}

	// === HEALING REWARDS (DOMINANT) ===
	float CurrentHPS = GetAverageHPS();
	float PrevHPS = 0.0f;  // TODO: Track previous HPS if needed
	float DeltaHPS = CurrentHPS - PrevHPS;

	// Base reward for healing (3x multiplier)
	Reward += CurrentHPS * 3.0f;

	// Delta bonus for healing
	Reward += DeltaHPS * 5.0f;

	// === DPS REWARDS (Lower priority) ===
	float CurrentDPS = GetAverageDPS();
	float DeltaDPS = CurrentDPS - PreviousDPS;

	Reward += CurrentDPS * 1.0f;  // Lower multiplier
	Reward += DeltaDPS * 5.0f;

	// === HEALTH DELTA ===
	float DeltaHealth = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
	Reward += DeltaHealth * 12.0f;  // High priority on staying alive

	// === STATIC BONUSES ===
	// Staying far from player
	if (CurrentState.DistanceToPlayer > 0.7f) {
		Reward += 2.0f;
	}

	// Penalty for being too close
	if (CurrentState.DistanceToPlayer < 0.3f) {
		Reward -= 3.0f;
	}

	// Near injured allies (ready to heal)
	float AvgAllyHealth = (CurrentState.HealthOfClosestAlly + 
		CurrentState.HealthOfSecondClosestAlly + 
		CurrentState.HealthOfThirdClosestAlly) / 3.0f;
	
	if (AvgAllyHealth < 0.7f && CurrentState.NumNearbyAllies > 0.2f) {
		Reward += 2.0f;
	}

	// Behind allies (safety)
	if (CurrentState.NumNearbyAllies > 0.2f &&
		CurrentState.DistanceToClosestAlly < CurrentState.DistanceToPlayer) {
		Reward += 1.5f;
	}

	// Penalty for taking damage (should avoid combat)
	if (CurrentState.bTookDamageRecently) {
		Reward -= 1.0f;
	}

	return Reward;
}
