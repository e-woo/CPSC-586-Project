#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SwarmAIController.generated.h"

/**
 * AI Controller for swarm enemies
 * Handles possession and basic AI logic for ASwarmEnemy
 */
UCLASS()
class SOULSTRIKE_API ASwarmAIController : public AAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this controller's properties
	ASwarmAIController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called when the controller possesses a pawn
	virtual void OnPossess(APawn* InPawn) override;

	// Called when the controller unpossesses a pawn
	virtual void OnUnPossess() override;

protected:
	/** Reference to the possessed swarm enemy */
	UPROPERTY(BlueprintReadOnly, Category = "Swarm AI")
	class ASwarmEnemy* ControlledEnemy;
};
