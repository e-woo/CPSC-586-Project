#include "SwarmEnemy.h"

// Sets default values
ASwarmEnemy::ASwarmEnemy()
{
 	// Set this character to call Tick() every frame. You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Initialize health values
	MaxHealth = 100.0f;
	Health = MaxHealth;

	// Set AI controller class (will be configured in Blueprint to use SwarmAIController)
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

// Called when the game starts or when spawned
void ASwarmEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	// Ensure health is set to max at start
	Health = MaxHealth;
}

// Called every frame
void ASwarmEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ASwarmEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ASwarmEnemy::TakeDamageCustom(float DamageAmount)
{
	if (!IsAlive())
	{
		return;
	}

	Health -= DamageAmount;
	Health = FMath::Clamp(Health, 0.0f, MaxHealth);

	if (!IsAlive())
	{
		// Enemy has died - can be extended with death logic
		// e.g., play death animation, disable collision, etc.
	}
}

bool ASwarmEnemy::IsAlive() const
{
	return Health > 0.0f;
}

float ASwarmEnemy::GetHealthPercent() const
{
	if (MaxHealth <= 0.0f)
	{
		return 0.0f;
	}
	
	return Health / MaxHealth;
}