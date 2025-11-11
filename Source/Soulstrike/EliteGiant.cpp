#include "EliteGiant.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RLComponent.h"
#include "DirectorAI.h"
#include "Kismet/GameplayStatics.h"

AEliteGiant::AEliteGiant()
{
	// Giant stats
	MaxHealth = 250.0f;  // Very high health
	Health = MaxHealth;
	AttackDamage = 200.0f;  // Massive damage per hit
	MaxAttackRange = 600.0f;  // Melee range

	// Giant attack timing: Slow devastating slams
	AttackDuration = 0.1f;  // Slam impact
	AttackCooldown = 1.9f;  // Very slow recovery

	// Movement
	GetCharacterMovement()->MaxWalkSpeed = 250.0f; // Very slow
}

void AEliteGiant::PerformPrimaryAttack()
{
	Super::PerformPrimaryAttack();

	// Giant heavy slam
	UE_LOG(LogTemp, Log, TEXT("Giant: SLAM! (%.0f damage)"), AttackDamage);

	// Record massive damage
	URLComponent* RLComp = FindComponentByClass<URLComponent>();
	if (RLComp)
	{
		RLComp->RecordDamageDealt(AttackDamage);
	}

	// Get player and Director AI
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	ADirectorAI* Director = Cast<ADirectorAI>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ADirectorAI::StaticClass())
	);

	// Report damage to Director
	if (Director && Player)
	{
		Director->ReportDamageToPlayer(Player, AttackDamage, this);
	}
}
