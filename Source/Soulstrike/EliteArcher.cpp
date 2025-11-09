#include "EliteArcher.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RLComponent.h"
#include "Kismet/GameplayStatics.h"

AEliteArcher::AEliteArcher()
{
	// Archer stats
	MaxHealth = 90.0f;
	Health = MaxHealth;
	AttackDamage = 50.0f;  // Burst damage per shot
	MaxAttackRange = 2000.0f;  // Long range

	// Archer attack timing: Fast shots
	// DPS = 50 / (0.4 + 0.1) = 100
	AttackDuration = 0.1f;  // Quick shot
	AttackCooldown = 0.4f;  // Fast fire rate

	// Movement
	GetCharacterMovement()->MaxWalkSpeed = 450.0f; // Medium speed
}

void AEliteArcher::PerformPrimaryAttack()
{
	Super::PerformPrimaryAttack();

	// Archer ranged attack
	UE_LOG(LogTemp, Log, TEXT("Archer: Fired arrow! (%.0f damage)"), AttackDamage);

	// Get RLComponent to record damage
	URLComponent* RLComp = FindComponentByClass<URLComponent>();
	if (RLComp)
	{
		RLComp->RecordDamageDealt(AttackDamage);
	}

	// TODO: Spawn projectile, play animation, etc.
	// For now, we conceptually "deal damage" to player
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (Player)
	{
		// Placeholder: Player->TakeDamageFromElite(AttackDamage, this);
	}
}
