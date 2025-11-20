#include "ElitePaladin.h"
#include "GameFramework/CharacterMovementComponent.h"

AElitePaladin::AElitePaladin()
{
	// Moved to Blueprint
}

void AElitePaladin::PerformPrimaryAttack()
{
	Super::PerformPrimaryAttack();
	UE_LOG(LogTemp, Log, TEXT("Paladin: Shield bash!"));
}
