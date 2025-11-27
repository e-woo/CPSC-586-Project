#include "GiantRLComponent.h"

float UGiantRLComponent::CalculateReward()
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
		// IN RANGE: Giant wants to stay close (tank)
		if (DeltaDistance < 0.0f) {  // Moving closer = good
			Reward += -DeltaDistance * 0.015f;  // 15.0 per 1000 units
		} else if (DeltaDistance > 0.0f) {  // Retreating = bad
			Reward -= DeltaDistance * 0.01f;  // 10.0 penalty per 1000 units
		}
	}

	// === DPS REWARDS (Base + Delta Bonus) ===
	float CurrentDPS = GetAverageDPS();
	float DeltaDPS = CurrentDPS - PreviousDPS;

	Reward += CurrentDPS * 2.0f;  // Base DPS
	Reward += DeltaDPS * 5.0f;    // Delta bonus

	// === HEALTH MANAGEMENT (High priority for tanks) ===
	float DeltaHealth = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
	Reward += DeltaHealth * 15.0f;  // Higher multiplier than other classes - survival is key

	// === TANKING BONUSES ===
	// Survival bonus (small static)
	Reward += CurrentState.SelfHealthPercentage * 1.0f;

	// Frontline position (very close to player)
	if (CurrentState.DistanceToPlayer < 0.3f) {
		Reward += 3.0f;
	}

	// Taking damage (tanking for team - drawing aggro)
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
