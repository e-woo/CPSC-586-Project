#include "EliteAssassin.h"
#include "GameFramework/CharacterMovementComponent.h"

AEliteAssassin::AEliteAssassin()
{
	// Moved to Blueprint
}

void AEliteAssassin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Poison is now handled by RLComponent
}

void AEliteAssassin::PerformPrimaryAttack()
{
	Super::PerformPrimaryAttack();
	UE_LOG(LogTemp, Log, TEXT("Assassin: Poisoned strike!"));
}
