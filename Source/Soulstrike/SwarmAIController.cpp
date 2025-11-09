#include "SwarmAIController.h"
#include "SwarmEnemy.h"

ASwarmAIController::ASwarmAIController()
{
	// Set this controller to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;

	ControlledEnemy = nullptr;
}

void ASwarmAIController::BeginPlay()
{
	Super::BeginPlay();
}

void ASwarmAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// AI logic will be primarily handled by the BoidsComponent
	// This controller mainly manages possession and high-level decisions
}

void ASwarmAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Cache reference to the controlled swarm enemy
	ControlledEnemy = Cast<ASwarmEnemy>(InPawn);

	if (ControlledEnemy)
	{
		// Successfully possessed a swarm enemy
		UE_LOG(LogTemp, Log, TEXT("SwarmAIController possessed: %s"), *ControlledEnemy->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SwarmAIController failed to possess ASwarmEnemy"));
	}
}

void ASwarmAIController::OnUnPossess()
{
	// Clear the reference
	ControlledEnemy = nullptr;

	Super::OnUnPossess();
}
