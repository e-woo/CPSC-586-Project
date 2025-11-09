#pragma once

#include "CoreMinimal.h"
#include "EliteEnemy.h"
#include "EliteAssassin.generated.h"

/** Poison damage-over-time effect */
USTRUCT()
struct FPoisonEffect
{
	GENERATED_BODY()

	float RemainingDuration;
	float DamagePerTick;
	float TickInterval;
	float TimeSinceLastTick;

	FPoisonEffect()
		: RemainingDuration(0.0f)
		, DamagePerTick(0.0f)
		, TickInterval(0.5f)
		, TimeSinceLastTick(0.0f)
	{}
};

/**
 * Elite Assassin - Fast melee attacker with poison
 * Prefers close-mid range combat
 */
UCLASS()
class SOULSTRIKE_API AEliteAssassin : public AEliteEnemy
{
	GENERATED_BODY()

public:
	AEliteAssassin();

	virtual void Tick(float DeltaTime) override;
	virtual void PerformPrimaryAttack() override;

protected:
	/** Active poison effects on player */
	TArray<FPoisonEffect> ActivePoisons;

	/** Apply poison to player */
	void ApplyPoison(float TotalDamage, float Duration);

	/** Update poison ticks */
	void UpdatePoisons(float DeltaTime);
};
