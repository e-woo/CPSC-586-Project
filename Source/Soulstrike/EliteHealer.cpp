#include "EliteHealer.h"
#include "GameFramework/CharacterMovementComponent.h"

AEliteHealer::AEliteHealer()
{
	// Moved to Blueprint
}

void AEliteHealer::PerformPrimaryAttack()
{
	Super::PerformPrimaryAttack();
	UE_LOG(LogTemp, Log, TEXT("Healer: Light attack"));
}

void AEliteHealer::PerformSecondaryAttack()
{
	Super::PerformSecondaryAttack();
	// Healing logic handled by RLComponent
}
