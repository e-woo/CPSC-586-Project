#include "AssassinRLComponent.h"

float UAssassinRLComponent::CalculateReward()
{
	if (!OwnerElite)
		return 0.0f;

	float Reward = 0.0f;

	// === DISTANCE DELTA (Conditional on Attack State) ===
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
		// IN RANGE: Hit-and-run tactics
		if (JustAttacked())  // Recently attacked, should retreat
		{
			if (DeltaDistance > 0.0f) {  // Retreating = good
				Reward += DeltaDistance * 30.0f;
			} else if (DeltaDistance < 0.0f) {  // Still closing = bad
				Reward += DeltaDistance * 20.0f;  // Penalty
			}
		}
		else if (CanAttack())  // Attack ready, should dive in
		{
			if (DeltaDistance < 0.0f) {  // Closing gap = good
				Reward += -DeltaDistance * 25.0f;
			}
		}
	}

	// === DPS REWARDS (Base + Delta Bonus) ===
	float CurrentDPS = GetAverageDPS();
	float DeltaDPS = CurrentDPS - PreviousDPS;

	// Base reward (high multiplier for burst damage)
	Reward += CurrentDPS * 3.0f;

	// Delta bonus
	Reward += DeltaDPS * 5.0f;

	// === HEALTH DELTA ===
	float DeltaHealth = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
	Reward += DeltaHealth * 10.0f;

	// === SMALL STATIC BONUSES ===
	// Mobility (strafing = dodging)
	if (LastAction == EEliteAction::Strafe_Left || LastAction == EEliteAction::Strafe_Right) {
		Reward += 0.5f;
	}

	// Using allies as cover when retreating
	if (JustAttacked() && CurrentState.NumNearbyAllies > 0.2f &&
		CurrentState.DistanceToClosestAlly < CurrentState.DistanceToPlayer) {
		Reward += 1.0f;
	}

	return Reward;
}
