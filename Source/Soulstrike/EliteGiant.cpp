#include "EliteGiant.h"
#include "GameFramework/CharacterMovementComponent.h"

AEliteGiant::AEliteGiant()
{
	// Moved to Blueprint
}

void AEliteGiant::PerformPrimaryAttack()
{
	Super::PerformPrimaryAttack();
	UE_LOG(LogTemp, Log, TEXT("Giant: SLAM!"));
}
