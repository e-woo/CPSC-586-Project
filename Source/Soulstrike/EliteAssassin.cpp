#include "EliteAssassin.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RLComponent.h"
#include "EnemyLogicManager.h"
#include "Kismet/GameplayStatics.h"

AEliteAssassin::AEliteAssassin()
{
	// Assassin stats
	MaxHealth = 80.0f;
	CurrentHealth = MaxHealth;
	AttackDamage = 10.0f;  // Initial hit (poison does 90 more)
	MaxAttackRange = 400.0f;  // Close range

	// Assassin attack timing: Quick hit then poison ticks
	AttackWindupDuration = 0.2f;  // Fast strike
	AttackCooldown = 1.5f;  // Wait for poison to work

	// Movement
	GetCharacterMovement()->MaxWalkSpeed = 600.0f; // Fast
}

void AEliteAssassin::Tick(float DeltaTime)
{
	// Poison is now handled by RLComponent, no need to update here
}

void AEliteAssassin::PerformPrimaryAttack()
{
	// Assassin melee attack - poison is applied by RLComponent's OnAttackWindupComplete
	// This method is not currently called (RLComponent handles attacks), but kept for consistency
	UE_LOG(LogTemp, Log, TEXT("Assassin: Poisoned strike! (%.0f instant + poison)"), AttackDamage);
}
