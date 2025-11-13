#include "ElitePaladin.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RLComponent.h"
#include "EnemyLogicManager.h"
#include "Kismet/GameplayStatics.h"

AElitePaladin::AElitePaladin()
{
	// Paladin stats
	MaxHealth = 150.0f;  // High health tank
	Health = MaxHealth;
	AttackDamage = 35.0f;  // Steady damage
	MaxAttackRange = 500.0f;  // Melee range

	// Paladin attack timing: Steady consistent attacks
	AttackDuration = 0.05f;  // Quick swing
	AttackCooldown = 0.3f;  // Steady pace

	// Movement
	GetCharacterMovement()->MaxWalkSpeed = 350.0f; // Slow
}

void AElitePaladin::PerformPrimaryAttack()
{
	Super::PerformPrimaryAttack();

	// Paladin melee attack
	UE_LOG(LogTemp, Log, TEXT("Paladin: Shield bash! (%.0f damage)"), AttackDamage);

	// Record damage
	URLComponent* RLComp = FindComponentByClass<URLComponent>();
	if (RLComp)
	{
		RLComp->RecordDamageDealt(AttackDamage);
	}

	// Get player and Enemy Logic Manager
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	AEnemyLogicManager* EnemyLogicMgr = Cast<AEnemyLogicManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AEnemyLogicManager::StaticClass())
	);

	// Report damage to Enemy Logic Manager
	if (EnemyLogicMgr && Player)
	{
		EnemyLogicMgr->ReportDamageToPlayer(Player, AttackDamage, this);
	}
}
