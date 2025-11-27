#pragma once

#include "CoreMinimal.h"
#include "RLComponent.h"

/**
 * Elite Stats Structure - holds all combat and movement stats
 */
struct FEliteStats
{
	float AttackDamage;
	float MaxAttackRange;
	float AttackWindupDuration;
	float AttackCooldown;
	float MovementSpeed;
	float HealAmount; // For Healer only
	
	FEliteStats()
		: AttackDamage(10.0f)
		, MaxAttackRange(500.0f)
		, AttackWindupDuration(0.3f)
		, AttackCooldown(0.5f)
		, MovementSpeed(400.0f)
		, HealAmount(50.0f)
	{}
};

/**
 * Q-Learning Brain - Handles all Q-value calculations, action selection, and weight updates
 * Extracted from RLComponent to reduce file size and improve maintainability
 */
class SOULSTRIKE_API FQLearningBrain
{
public:
	FQLearningBrain();
	~FQLearningBrain();

	// ========== INITIALIZATION ==========
	
	/** Initialize weights for all actions and features with biased initialization */
	void InitializeWeights();

	/** Load weights from external storage (shared across elites of same type) */
	void LoadWeights(const TMap<EEliteAction, TMap<FName, float>>& InWeights);

	/** Get current weights for saving */
	const TMap<EEliteAction, TMap<FName, float>>& GetWeights() const { return Weights; }

	// ========== STATS POLLING ==========
	
	/** Read all elite stats from Blueprint properties */
	static FEliteStats ReadStatsFromBlueprint(class ACharacter* Character);

	// ========== Q-LEARNING CORE ==========

	/** Calculate Q-value for a given state-action pair */
	float CalculateQValue(const FRLState& State, EEliteAction Action) const;

	/** Select an action using epsilon-greedy policy */
	EEliteAction SelectAction(const FRLState& State, float Epsilon) const;

	/** Update Q-learning weights based on the transition */
	void UpdateWeights(const FRLState& OldState, EEliteAction Action, float Reward, const FRLState& NewState, float Alpha, float Gamma);

	/** Extract feature values from a state */
	static TMap<FName, float> ExtractFeatures(const FRLState& State);

private:
	/** Weights for Q-value calculation: Action -> (FeatureName -> Weight) */
	TMap<EEliteAction, TMap<FName, float>> Weights;

	/** Feature names used in Q-learning */
	static TArray<FName> GetFeatureNames();
};
