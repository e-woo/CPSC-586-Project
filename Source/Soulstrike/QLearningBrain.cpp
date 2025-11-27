#include "QLearningBrain.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

FQLearningBrain::FQLearningBrain()
{
}

FQLearningBrain::~FQLearningBrain()
{
}

FEliteStats FQLearningBrain::ReadStatsFromBlueprint(ACharacter* Character)
{
	FEliteStats Stats;
	
	if (!Character)
		return Stats;

	UClass* CharacterClass = Character->GetClass();
	if (!CharacterClass)
		return Stats;

	// Helper lambda to read float property
	auto ReadFloatProperty = [CharacterClass](ACharacter* Char, const FName& PropertyName, float DefaultValue) -> float {
		FProperty* Property = CharacterClass->FindPropertyByName(PropertyName);
		if (Property)
		{
			float* ValuePtr = Property->ContainerPtrToValuePtr<float>(Char);
			if (ValuePtr)
				return *ValuePtr;
		}
		return DefaultValue;
	};

	// Read all stats
	Stats.AttackDamage = ReadFloatProperty(Character, TEXT("AttackDamage"), 10.0f);
	Stats.MaxAttackRange = ReadFloatProperty(Character, TEXT("MaxAttackRange"), 500.0f);
	Stats.AttackWindupDuration = ReadFloatProperty(Character, TEXT("AttackWindupDuration"), 0.3f);
	Stats.AttackCooldown = ReadFloatProperty(Character, TEXT("AttackCooldown"), 0.5f);
	Stats.MovementSpeed = ReadFloatProperty(Character, TEXT("MovementSpeed"), 400.0f);
	Stats.HealAmount = ReadFloatProperty(Character, TEXT("HealAmount"), 50.0f);

	return Stats;
}

void FQLearningBrain::InitializeWeights()
{
	// Initialize weights for all actions
	TArray<EEliteAction> Actions = {
		EEliteAction::Move_Towards_Player,
		EEliteAction::Move_Away_From_Player,
		EEliteAction::Strafe_Left,
		EEliteAction::Strafe_Right,
		EEliteAction::Primary_Attack,
		EEliteAction::Secondary_Attack
	};

	TArray<FName> FeatureNames = GetFeatureNames();

	for (EEliteAction Action : Actions)
	{
		TMap<FName, float> ActionWeights;
		for (FName FeatureName : FeatureNames)
		{
			// Initialize weights to small random values
			ActionWeights.Add(FeatureName, FMath::FRandRange(-0.1f, 0.1f));
		}
		Weights.Add(Action, ActionWeights);
	}

	// === BIASED INITIALIZATION FOR ENGAGEMENT ===
	// All elites should move toward player when out of range
	Weights[EEliteAction::Move_Towards_Player]["bIsBeyondMaxRange"] = +1.0f;  // Strong positive
	Weights[EEliteAction::Move_Towards_Player]["DistanceToPlayer"] = +0.5f;   // Mild bias toward closing gap

	// Discourage moving away when out of range
	Weights[EEliteAction::Move_Away_From_Player]["bIsBeyondMaxRange"] = -1.0f;  // Strong negative

	// Strafe is neutral/slightly negative when out of range
	Weights[EEliteAction::Strafe_Left]["bIsBeyondMaxRange"] = -0.5f;
	Weights[EEliteAction::Strafe_Right]["bIsBeyondMaxRange"] = -0.5f;

	// Bias toward attacking when cooldown ready and in range
	Weights[EEliteAction::Primary_Attack]["TimeSinceLastAttack"] = +0.8f;  // Attack when ready
	Weights[EEliteAction::Primary_Attack]["DistanceToPlayer"] = -0.5f;  // Prefer when close (low distance value)
	Weights[EEliteAction::Primary_Attack]["bIsBeyondMaxRange"] = -0.8f;  // Don't attack when out of range
}

void FQLearningBrain::LoadWeights(const TMap<EEliteAction, TMap<FName, float>>& InWeights)
{
	Weights = InWeights;
}

float FQLearningBrain::CalculateQValue(const FRLState& State, EEliteAction Action) const
{
	if (!Weights.Contains(Action))
		return 0.0f;

	TMap<FName, float> Features = ExtractFeatures(State);
	const TMap<FName, float>& ActionWeights = Weights[Action];

	float QValue = 0.0f;
	for (const auto& FeaturePair : Features)
	{
		FName FeatureName = FeaturePair.Key;
		float FeatureValue = FeaturePair.Value;

		if (ActionWeights.Contains(FeatureName))
		{
			QValue += ActionWeights[FeatureName] * FeatureValue;
		}
	}

	return QValue;
}

EEliteAction FQLearningBrain::SelectAction(const FRLState& State, float Epsilon) const
{
	// Epsilon-greedy policy
	float RandomValue = FMath::FRand();
	if (RandomValue < Epsilon)
	{
		// Explore: choose random action
		int32 RandomIndex = FMath::RandRange(0, 5); // 6 actions
		return static_cast<EEliteAction>(RandomIndex);
	}
	else
	{
		// Exploit: choose action with highest Q-value
		EEliteAction BestAction = EEliteAction::Move_Towards_Player;
		float BestQValue = -FLT_MAX;

		for (int32 i = 0; i < 6; ++i)
		{
			EEliteAction Action = static_cast<EEliteAction>(i);
			float QValue = CalculateQValue(State, Action);
			if (QValue > BestQValue)
			{
				BestQValue = QValue;
				BestAction = Action;
			}
		}

		return BestAction;
	}
}

void FQLearningBrain::UpdateWeights(const FRLState& OldState, EEliteAction Action, float Reward, const FRLState& NewState, float Alpha, float Gamma)
{
	if (!Weights.Contains(Action))
		return;

	// Calculate current Q-value for old state-action
	float OldQValue = CalculateQValue(OldState, Action);

	// Find max Q-value for new state
	float MaxNewQValue = -FLT_MAX;
	for (int32 i = 0; i < 6; ++i)
	{
		EEliteAction NextAction = static_cast<EEliteAction>(i);
		float QValue = CalculateQValue(NewState, NextAction);
		MaxNewQValue = FMath::Max(MaxNewQValue, QValue);
	}

	// TD Error: reward + gamma * max(Q(s',a')) - Q(s,a)
	float TDError = Reward + (Gamma * MaxNewQValue) - OldQValue;

	// Update weights: w = w + alpha * TDError * feature_value
	TMap<FName, float> Features = ExtractFeatures(OldState);
	TMap<FName, float>& ActionWeights = Weights[Action];

	for (const auto& FeaturePair : Features)
	{
		FName FeatureName = FeaturePair.Key;
		float FeatureValue = FeaturePair.Value;

		if (ActionWeights.Contains(FeatureName))
		{
			ActionWeights[FeatureName] += Alpha * TDError * FeatureValue;
		}
	}
}

TMap<FName, float> FQLearningBrain::ExtractFeatures(const FRLState& State)
{
	TMap<FName, float> Features;

	Features.Add("DistanceToPlayer", State.DistanceToPlayer);
	Features.Add("SelfHealthPercentage", State.SelfHealthPercentage);
	Features.Add("TimeSinceLastAttack", State.TimeSinceLastAttack);
	Features.Add("bIsBeyondMaxRange", State.bIsBeyondMaxRange ? 1.0f : 0.0f);
	Features.Add("bTookDamageRecently", State.bTookDamageRecently ? 1.0f : 0.0f);
	Features.Add("PlayerHealthPercentage", State.PlayerHealthPercentage);
	Features.Add("bHasLineOfSightToPlayer", State.bHasLineOfSightToPlayer ? 1.0f : 0.0f);
	Features.Add("HealthOfClosestAlly", State.HealthOfClosestAlly);
	Features.Add("DistanceToClosestAlly", State.DistanceToClosestAlly);
	Features.Add("HealthOfSecondClosestAlly", State.HealthOfSecondClosestAlly);
	Features.Add("DistanceToSecondClosestAlly", State.DistanceToSecondClosestAlly);
	Features.Add("HealthOfThirdClosestAlly", State.HealthOfThirdClosestAlly);
	Features.Add("DistanceToThirdClosestAlly", State.DistanceToThirdClosestAlly);
	Features.Add("NumNearbyAllies", State.NumNearbyAllies);

	return Features;
}

TArray<FName> FQLearningBrain::GetFeatureNames()
{
	return {
		"DistanceToPlayer",
		"SelfHealthPercentage",
		"TimeSinceLastAttack",
		"bIsBeyondMaxRange",
		"bTookDamageRecently",
		"PlayerHealthPercentage",
		"bHasLineOfSightToPlayer",
		"HealthOfClosestAlly",
		"DistanceToClosestAlly",
		"HealthOfSecondClosestAlly",
		"DistanceToSecondClosestAlly",
		"HealthOfThirdClosestAlly",
		"DistanceToThirdClosestAlly",
		"NumNearbyAllies"
	};
}
