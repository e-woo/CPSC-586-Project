#include "EliteHealer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RLComponent.h"
#include "EnemyLogicManager.h"
#include "Kismet/GameplayStatics.h"

AEliteHealer::AEliteHealer()
{
	// Healer stats
	MaxHealth = 70.0f;  // Low health
	CurrentHealth = MaxHealth;
	AttackDamage = 15.0f;  // Weak attack
	MaxAttackRange = 1500.0f;  // Medium range

	// Healer attack timing: Weak attacks
	AttackWindupDuration = 0.1f;
	AttackCooldown = 0.4f;

	// Heal ability
	HealAmount = 50.0f;  // Heals 50 HP

	// Movement
	GetCharacterMovement()->MaxWalkSpeed = 400.0f; // Medium speed
}

void AEliteHealer::PerformPrimaryAttack()
{
	Super::PerformPrimaryAttack();

	// Healer weak ranged attack
	UE_LOG(LogTemp, Log, TEXT("Healer: Light attack (%.0f damage)"), AttackDamage);

	// Record weak damage
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

void AEliteHealer::PerformSecondaryAttack()
{
	Super::PerformSecondaryAttack();

	// Healer heal ability - find injured ally
	URLComponent* RLComp = FindComponentByClass<URLComponent>();
	if (!RLComp)
		return;

	// Find lowest HP ally from the 3 closest
	TArray<ACharacter*> ClosestAllies;
	RLComp->FindClosestAllies(ClosestAllies, 3);

	ACharacter* BestTarget = nullptr;
	float LowestHP = 1.0f;

	for (ACharacter* Ally : ClosestAllies)
	{
		if (Ally && RLComp->GetCharacterHealthPercentage(Ally) > 0.0f)
		{
			float AllyHP = RLComp->GetCharacterHealthPercentage(Ally);
			if (AllyHP < 0.9f && AllyHP < LowestHP)
			{
				float Distance = FVector::Dist(GetActorLocation(), Ally->GetActorLocation());
				if (Distance <= MaxAttackRange)
				{
					LowestHP = AllyHP;
					BestTarget = Ally;
				}
			}
		}
	}

	if (BestTarget)
	{
		// Heal the ally
		float CurrentHealth, MaxHealth;
		if (RLComp->GetCharacterHealth(BestTarget, CurrentHealth) && RLComp->GetCharacterMaxHealth(BestTarget, MaxHealth))
		{
			float NewHealth = FMath::Min(MaxHealth, CurrentHealth + HealAmount);
			RLComp->SetCharacterHealth(BestTarget, NewHealth);
		}
		RLComp->RecordHealingDone(HealAmount);
		
		UE_LOG(LogTemp, Log, TEXT("Healer: Healed %s for %.0f HP"), *BestTarget->GetName(), HealAmount);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Healer: No valid heal target found!"));
	}
}
