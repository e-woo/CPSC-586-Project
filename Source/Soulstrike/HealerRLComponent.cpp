#include "HealerRLComponent.h"
#include "GameFramework/Character.h"

void UHealerRLComponent::PerformSecondaryAttackOnElite()
{
	if (!OwnerCharacter)
		return;

	// Read HealAmount from Blueprint
	FProperty* HealAmountProp = OwnerCharacter->GetClass()->FindPropertyByName(TEXT("HealAmount"));
	float HealAmount = 50.0f; // Default
	if (HealAmountProp)
	{
		float* HealAmountPtr = HealAmountProp->ContainerPtrToValuePtr<float>(OwnerCharacter);
		if (HealAmountPtr)
		{
			HealAmount = *HealAmountPtr;
		}
	}

	// Find lowest HP ally and heal them
	TArray<ACharacter*> ClosestAllies;
	FindClosestAllies(ClosestAllies, 3);

	ACharacter* BestTarget = nullptr;
	float LowestHP = 1.0f;

	for (ACharacter* Ally : ClosestAllies)
	{
		if (Ally && Ally->IsValidLowLevel() && !Ally->IsPendingKillOrUnreachable() && 
			IsCharacterAlive(Ally))
		{
			float AllyHP = GetCharacterHealthPercentage(Ally);
			if (AllyHP < 0.9f && AllyHP < LowestHP)
			{
				float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), Ally->GetActorLocation());
				if (Distance <= MaxAttackRange)
				{
					LowestHP = AllyHP;
					BestTarget = Ally;
				}
			}
		}
	}

	if (BestTarget && BestTarget->IsValidLowLevel() && !BestTarget->IsPendingKillOrUnreachable())
	{
		// Set attack state (before triggering animation)
		AttackState = EAttackState::Attacking;
		AttackTimer = 0.0f;
		
		// Trigger Blueprint custom event "OnSecondaryStart" with WindupDuration parameter
		UFunction* SecondaryStartFunc = OwnerCharacter->FindFunction(FName("OnSecondaryStart"));
		if (SecondaryStartFunc)
		{
			struct FSecondaryStartParams
			{
				float WindupDuration;
			};
			
			FSecondaryStartParams Params;
			Params.WindupDuration = AttackWindupDuration;
			
			OwnerCharacter->ProcessEvent(SecondaryStartFunc, &Params);
		}

		// Heal the ally
		UClass* TargetClass = BestTarget->GetClass();
		if (TargetClass && TargetClass->IsValidLowLevel())
		{
			FProperty* CurrentHealthProp = TargetClass->FindPropertyByName(TEXT("CurrentHealth"));
			FProperty* MaxHealthProp = TargetClass->FindPropertyByName(TEXT("MaxHealth"));
			if (CurrentHealthProp && MaxHealthProp)
			{
				float* CurrentHealthPtr = CurrentHealthProp->ContainerPtrToValuePtr<float>(BestTarget);
				float* MaxHealthPtr = MaxHealthProp->ContainerPtrToValuePtr<float>(BestTarget);
				if (CurrentHealthPtr && MaxHealthPtr)
				{
					*CurrentHealthPtr = FMath::Min(*MaxHealthPtr, *CurrentHealthPtr + HealAmount);
				}
			}
		}
		
		RecordHealingDone(HealAmount);
		UE_LOG(LogTemp, Log, TEXT("Healer: Healed %s for %.0f HP"), *BestTarget->GetName(), HealAmount);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Healer: No valid heal target found"));
	}
}

float UHealerRLComponent::CalculateReward()
{
	if (!OwnerCharacter)
		return 0.0f;

	float Reward = 0.0f;

	// Calculate distance delta (using actual distance)
	float PrevActualDistance = PreviousDistanceToPlayer * MaxAttackRange;
	float DeltaDistance = ActualDistanceToPlayer - PrevActualDistance;

	// === DISTANCE MANAGEMENT (Special Case for Healer) ===
	if (CurrentState.bIsBeyondMaxRange)
	{
		// SPECIAL: Only penalize if ALSO out of range of allies
		bool InRangeOfAlly = (CurrentState.DistanceToClosestAlly < 0.5f);
		
		if (!InRangeOfAlly)  // Out of range of BOTH player and allies = bad
		{
			if (DeltaDistance < 0.0f) {
				Reward += -DeltaDistance * 0.05f;  // Move toward someone
			} else if (DeltaDistance > 0.0f) {
				Reward -= DeltaDistance * 0.05f;
			} else {
				Reward -= 5.0f;
			}
		}
	}
	else
	{
		// IN RANGE: Increase distance (backline safety)
		if (DeltaDistance > 0.0f) {  // Moving away = good
			Reward += DeltaDistance * 0.02f;  // 20.0 per 1000 units
		} else if (DeltaDistance < 0.0f) {  // Getting closer = bad
			Reward -= -DeltaDistance * 0.015f;  // 15.0 per 1000 units
		}
	}

	// === HEALING REWARDS (DOMINANT) ===
	float CurrentHPS = GetAverageHPS();
	float DeltaHPS = CurrentHPS - PreviousHPS;

	// Base reward for healing (3x multiplier - highest priority)
	Reward += CurrentHPS * 3.0f;

	// Delta bonus for healing
	Reward += DeltaHPS * 5.0f;

	// === DPS REWARDS (Lower priority) ===
	float CurrentDPS = GetAverageDPS();
	float DeltaDPS = CurrentDPS - PreviousDPS;

	Reward += CurrentDPS * 1.0f;  // Lower multiplier than healing
	Reward += DeltaDPS * 5.0f;

	// === HEALTH MANAGEMENT (Very high priority) ===
	float DeltaHealth = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
	Reward += DeltaHealth * 12.0f;  // Must stay alive to heal

	// === POSITIONING BONUSES ===
	if (CurrentState.DistanceToPlayer > 0.7f)
		Reward += 2.0f;  // Far from player

	if (CurrentState.DistanceToPlayer < 0.3f)
		Reward -= 3.0f;  // Penalty for too close

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
