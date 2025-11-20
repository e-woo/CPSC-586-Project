#pragma once

#include "CoreMinimal.h"
#include "EliteEnemy.h"
#include "EliteArcher.generated.h"

/**
 * Elite Archer - Long range attacker
 * Prefers to keep distance from player
 */
UCLASS()
class SOULSTRIKE_API AEliteArcher : public AEliteEnemy
{
	GENERATED_BODY()

public:
	AEliteArcher();
};
