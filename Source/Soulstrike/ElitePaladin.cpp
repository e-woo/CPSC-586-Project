#include "ElitePaladin.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RLComponent.h"
#include "DirectorAI.h"
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
	
	// Initialize cached reference
	CachedDirector = nullptr;
}

void AElitePaladin::BeginPlay()
{
	Super::BeginPlay();
	
	// Cache DirectorAI reference during initialization
	CachedDirector = Cast<ADirectorAI>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ADirectorAI::StaticClass())
	);
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

	// Get player and use cached Director AI reference
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	// Report damage to Director using cached reference
	if (CachedDirector && Player)
	{
		CachedDirector->ReportDamageToPlayer(Player, AttackDamage, this);
	}
}
