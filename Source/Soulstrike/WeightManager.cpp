#include "WeightManager.h"

UWeightManager* UWeightManager::Instance = nullptr;

UWeightManager* UWeightManager::Get(UWorld* World)
{
	if (!Instance && World)
	{
		Instance = NewObject<UWeightManager>();
		Instance->AddToRoot(); // Prevent garbage collection
		UE_LOG(LogTemp, Log, TEXT("WeightManager: Singleton instance created."));
	}
	return Instance;
}

bool UWeightManager::HasWeights(EEliteType Type) const
{
	return StoredWeights.Contains(Type);
}

TMap<EEliteAction, TMap<FName, float>> UWeightManager::LoadWeights(EEliteType Type) const
{
	if (StoredWeights.Contains(Type))
	{
		UE_LOG(LogTemp, Log, TEXT("WeightManager: Loading weights for elite type %d (learned from previous souls)"), (int32)Type);
		return StoredWeights[Type];
	}

	UE_LOG(LogTemp, Warning, TEXT("WeightManager: No weights found for elite type %d (first spawn)"), (int32)Type);
	return TMap<EEliteAction, TMap<FName, float>>();
}

void UWeightManager::SaveWeights(EEliteType Type, const TMap<EEliteAction, TMap<FName, float>>& Weights)
{
	StoredWeights.Add(Type, Weights);
	UE_LOG(LogTemp, Log, TEXT("WeightManager: Saved weights for elite type %d (soul preserved)"), (int32)Type);
}

void UWeightManager::ResetAllWeights()
{
	int32 NumTypesReset = StoredWeights.Num();
	StoredWeights.Empty();
	
	if (NumTypesReset > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("WeightManager: All Elite AI weights reset! Cleared learned knowledge from %d elite type(s). Elites will start fresh."), NumTypesReset);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("WeightManager: Weight reset called (no learned weights to clear)."));
	}
}
