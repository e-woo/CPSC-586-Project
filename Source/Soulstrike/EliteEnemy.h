#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EliteEnemy.generated.h"

/**
 * Base class for all Elite Enemy types.
 * Provides core stats: Health, MaxHealth, AttackDamage, and MaxAttackRange.
 */
UCLASS()
class SOULSTRIKE_API AEliteEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AEliteEnemy();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ========== STATS ==========
	
	/** Current health of the elite */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Health;

	/** Maximum health of the elite */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float MaxHealth;

	/** Base attack damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float AttackDamage;

	/** Maximum attack range in Unreal units (used for distance normalization in RL) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float MaxAttackRange;

	// ========== ATTACK TIMING ==========

	/** How long the attack animation/action takes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float AttackDuration;

	/** Cooldown between attacks */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float AttackCooldown;

	/** Heal amount (for Healer's secondary attack) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float HealAmount;

	// ========== METHODS ==========

	/** Apply damage to this elite */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void TakeDamageFromPlayer(float DamageAmount);

	/** Heal this elite */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void Heal(float Amount);

	/** Get health percentage [0,1] */
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetHealthPercentage() const;

	/** Check if this elite is alive */
	UFUNCTION(BlueprintPure, Category = "Stats")
	bool IsAlive() const;

	// ========== ATTACK METHODS ==========

	/** Perform primary attack (virtual - override per elite type) */
	virtual void PerformPrimaryAttack();

	/** Perform secondary attack (virtual - override for special abilities) */
	virtual void PerformSecondaryAttack();

	/** Called when attack animation completes */
	virtual void OnAttackComplete();

protected:
	/** Called when health reaches zero */
	virtual void Die();
};
