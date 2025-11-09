#include "EliteHealer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RLComponent.h"
#include "Kismet/GameplayStatics.h"

AEliteHealer::AEliteHealer()
{
	// Healer stats
	MaxHealth = 70.0f;  // Low health
	Health = MaxHealth;
	AttackDamage = 15.0f;  // Weak attack
	MaxAttackRange = 1500.0f;  // Medium range

	// Healer attack timing: Weak attacks
	AttackDuration = 0.1f;
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

	// TODO: Player damage
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (Player)
	{
		// Placeholder: Player->TakeDamageFromElite(AttackDamage, this);
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
	TArray<AEliteEnemy*> ClosestAllies;
	RLComp->FindClosestAllies(ClosestAllies, 3);

	AEliteEnemy* BestTarget = nullptr;
	float LowestHP = 1.0f;

	for (AEliteEnemy* Ally : ClosestAllies)
	{
		if (Ally && Ally->IsAlive())
		{
			float AllyHP = Ally->GetHealthPercentage();
			if (AllyHP < 0.9f && AllyHP < LowestHP)  // Needs healing
			{
				float Distance = FVector::Dist(GetActorLocation(), Ally->GetActorLocation());
				if (Distance <= MaxAttackRange)  // In heal range
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
		BestTarget->Heal(HealAmount);
		RLComp->RecordHealingDone(HealAmount);
		
		UE_LOG(LogTemp, Log, TEXT("Healer: Healed %s for %.0f HP"), *BestTarget->GetName(), HealAmount);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Healer: No valid heal target found!"));
	}
}
