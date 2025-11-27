#pragma once

#include "CoreMinimal.h"
#include "RLComponent.h"
#include "WeightManager.generated.h"

/**
 * Elite Type Enumeration for Weight Management
 */
UENUM(BlueprintType)
enum class EEliteType : uint8
{
	Archer UMETA(DisplayName = "Archer"),
	Assassin UMETA(DisplayName = "Assassin"),
	Giant UMETA(DisplayName = "Giant"),
	Paladin UMETA(DisplayName = "Paladin"),
	Healer UMETA(DisplayName = "Healer")
};

/**
 * Weight Manager - Singleton that persists Q-learning weights per elite type
 * When an elite dies, its weights are saved here. When a new elite of the same type spawns,
 * it loads the saved weights, allowing "learning from the souls of the dead"
 */
UCLASS()
class SOULSTRIKE_API UWeightManager : public UObject
{
	GENERATED_BODY()

public:
	/** Get the singleton instance */
	static UWeightManager* Get(UWorld* World);

	/** Check if weights exist for a given elite type */
	bool HasWeights(EEliteType Type) const;

	/** Load weights for a given elite type */
	TMap<EEliteAction, TMap<FName, float>> LoadWeights(EEliteType Type) const;

	/** Save weights for a given elite type */
	void SaveWeights(EEliteType Type, const TMap<EEliteAction, TMap<FName, float>>& Weights);

	/** Reset all weights (for debugging/testing) */
	UFUNCTION(BlueprintCallable, Category = "RL|Debug")
	void ResetAllWeights();

private:
	/** Stored weights per elite type */
	TMap<EEliteType, TMap<EEliteAction, TMap<FName, float>>> StoredWeights;

	/** Singleton instance */
	static UWeightManager* Instance;
};
