#pragma once

#include "CoreMinimal.h"
#include "EliteEnemy.h"
#include "EliteAssassin.generated.h"

/**
 * Elite Assassin - Fast melee attacker with poison
 * Prefers close-mid range combat
 * Poison is handled by RLComponent
 */
UCLASS()
class SOULSTRIKE_API AEliteAssassin : public AEliteEnemy
{
	GENERATED_BODY()

public:
	AEliteAssassin();

	virtual void Tick(float DeltaTime) override;
	virtual void PerformPrimaryAttack() override;
};
