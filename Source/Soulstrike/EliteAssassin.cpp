#include "EliteAssassin.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RLComponent.h"
#include "DirectorAI.h"
#include "Kismet/GameplayStatics.h"

AEliteAssassin::AEliteAssassin()
{
	// Assassin stats
	MaxHealth = 80.0f;
	Health = MaxHealth;
	AttackDamage = 10.0f;  // Initial hit (poison does 90 more)
	MaxAttackRange = 400.0f;  // Close range

	// Assassin attack timing: Quick hit then poison ticks
	AttackDuration = 0.2f;  // Fast strike
	AttackCooldown = 1.5f;  // Wait for poison to work

	// Movement
	GetCharacterMovement()->MaxWalkSpeed = 600.0f; // Fast
}

void AEliteAssassin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update poison ticks
	UpdatePoisons(DeltaTime);
}

void AEliteAssassin::PerformPrimaryAttack()
{
	Super::PerformPrimaryAttack();

	// Assassin melee + poison attack
	UE_LOG(LogTemp, Log, TEXT("Assassin: Poisoned strike! (%.0f instant + 90 poison over 3s)"), AttackDamage);

	// Get components
	URLComponent* RLComp = FindComponentByClass<URLComponent>();
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	ADirectorAI* Director = Cast<ADirectorAI>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ADirectorAI::StaticClass())
	);

	// Record and report instant damage
	if (RLComp)
	{
		RLComp->RecordDamageDealt(AttackDamage);
	}

	if (Director && Player)
	{
		Director->ReportDamageToPlayer(Player, AttackDamage, this);
	}

	// Apply poison (90 damage over 3 seconds)
	ApplyPoison(90.0f, 3.0f);
}

void AEliteAssassin::ApplyPoison(float TotalDamage, float Duration)
{
	FPoisonEffect NewPoison;
	NewPoison.RemainingDuration = Duration;
	NewPoison.TickInterval = 0.5f;  // Tick every 0.5 seconds
	NewPoison.DamagePerTick = TotalDamage / (Duration / NewPoison.TickInterval);  // 90 / 6 = 15 per tick
	NewPoison.TimeSinceLastTick = 0.0f;

	ActivePoisons.Add(NewPoison);
}

void AEliteAssassin::UpdatePoisons(float DeltaTime)
{
	URLComponent* RLComp = FindComponentByClass<URLComponent>();
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	ADirectorAI* Director = Cast<ADirectorAI>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ADirectorAI::StaticClass())
	);

	for (int32 i = ActivePoisons.Num() - 1; i >= 0; --i)
	{
		FPoisonEffect& Poison = ActivePoisons[i];
		
		Poison.TimeSinceLastTick += DeltaTime;
		Poison.RemainingDuration -= DeltaTime;

		// Tick damage
		if (Poison.TimeSinceLastTick >= Poison.TickInterval)
		{
			// Record DPS for RL
			if (RLComp)
			{
				RLComp->RecordDamageDealt(Poison.DamagePerTick);
			}

			// Report poison damage to Director
			if (Director && Player)
			{
				Director->ReportDamageToPlayer(Player, Poison.DamagePerTick, this);
			}
			
			Poison.TimeSinceLastTick = 0.0f;
		}

		// Remove expired poisons
		if (Poison.RemainingDuration <= 0.0f)
		{
			ActivePoisons.RemoveAt(i);
		}
	}
}
