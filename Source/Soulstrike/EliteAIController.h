#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EliteAIController.generated.h"

class URLComponent;

/**
 * AI Controller for Elite Enemies.
 * Drives the RL component tick and manages AI execution.
 */
UCLASS()
class SOULSTRIKE_API AEliteAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEliteAIController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

public:
	virtual void Tick(float DeltaTime) override;

	/** Reference to the RL Component that drives behavior */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	URLComponent* RLComponent;

	/** How often (in seconds) to run the RL decision loop. 0 = every tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Performance")
	float RLTickInterval;

	/** Enable debug visualization (state, action, reward above character) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Debug")
	bool bEnableDebugMode;

	// ========== RL HYPERPARAMETERS (Convenience Properties) =========
	
	/** Learning rate (alpha) - How much to update Q-values each step */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|RL Tuning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LearningRate;

	/** Discount factor (gamma) - How much to value future rewards */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|RL Tuning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DiscountFactor;

	/** Exploration rate (epsilon) - Chance of random action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|RL Tuning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExplorationRate;

	/** Epsilon decay rate per second (0 = no decay) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|RL Tuning", meta = (ClampMin = "0.0"))
	float ExplorationDecayRate;

	/** How long to hold each action for smoother movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement", meta = (ClampMin = "0.0"))
	float ActionPersistenceDuration;

private:
	/** Time accumulator for RL tick */
	float RLTickAccumulator;
};
