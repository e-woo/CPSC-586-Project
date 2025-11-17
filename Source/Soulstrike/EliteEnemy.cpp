#include "EliteEnemy.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEliteEnemy::AEliteEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	// Default stats
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	AttackDamage = 10.0f;
	MaxAttackRange = 500.0f;

	// Default attack timing (will be overridden by subclasses)
	AttackWindupDuration = 0.1f;
	AttackCooldown = 0.5f;
	HealAmount = 0.0f;

	// Configure character movement
	UCharacterMovementComponent* CharMoveComp = GetCharacterMovement();
	if (CharMoveComp)
	{
		CharMoveComp->MaxWalkSpeed = 400.0f;
		CharMoveComp->bOrientRotationToMovement = true;
		CharMoveComp->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
		CharMoveComp->bUseControllerDesiredRotation = false;
		CharMoveComp->bConstrainToPlane = true;
		CharMoveComp->bSnapToPlaneAtStart = true;
		CharMoveComp->GravityScale = 1.0f;
		CharMoveComp->SetWalkableFloorAngle(45.0f);
	}

	// Configure capsule
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	if (CapsuleComp)
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CapsuleComp->SetCollisionResponseToAllChannels(ECR_Block);
		CapsuleComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		CapsuleComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	// AI configuration
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = nullptr; // Will be set in Blueprint
}

void AEliteEnemy::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
}

void AEliteEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEliteEnemy::TakeDamageFromPlayer(float DamageAmount)
{
	if (!IsAlive())
		return;

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);

	if (CurrentHealth <= 0.0f)
	{
		Die();
	}
}

void AEliteEnemy::Heal(float Amount)
{
	if (!IsAlive())
		return;

	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + Amount);
}

float AEliteEnemy::GetHealthPercentage() const
{
	if (MaxHealth <= 0.0f)
		return 0.0f;
	return FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f);
}

bool AEliteEnemy::IsAlive() const
{
	return CurrentHealth > 0.0f;
}

void AEliteEnemy::Die()
{
	// Log death
	UE_LOG(LogTemp, Warning, TEXT("Elite %s has died."), *GetName());

	// Disable collision and movement
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();

	// Destroy after a short delay
	SetLifeSpan(3.0f);
}

void AEliteEnemy::PerformPrimaryAttack()
{
	// Base implementation: log attack
	// Subclasses will override to implement specific attacks
	UE_LOG(LogTemp, Log, TEXT("%s: Performing primary attack (base)"), *GetName());
}

void AEliteEnemy::PerformSecondaryAttack()
{
	// Base implementation: do nothing
	// Subclasses will override for special abilities
	UE_LOG(LogTemp, Log, TEXT("%s: Performing secondary attack (base)"), *GetName());
}

void AEliteEnemy::OnAttackComplete()
{
	// Called when attack animation finishes
	// Can be used to apply damage/effects at the end of attack
}
