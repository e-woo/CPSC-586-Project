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

private:
	void MoveSwarms();
	
	static TMap<FString, TArray<TWeakObjectPtr<ACharacter>>> SwarmMap;

	const float SeparationDistance = 600.f;
	const float CohesionWeight = 0.8f;
	const float AlignmentWeight = 0.5f;
	const float SeparationWeight = 0.7f;
	const float TargetWeight = 0.5f;

	const float TargetEngageDistance = 2000.f;
};
