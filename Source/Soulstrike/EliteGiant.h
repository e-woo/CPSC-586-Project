#pragma once

#include "CoreMinimal.h"
#include "EliteEnemy.h"
#include "EliteGiant.generated.h"

/**
 * Elite Giant - Super tank
 * Highest health, very slow, deals massive damage
 */
UCLASS()
class SOULSTRIKE_API AEliteGiant : public AEliteEnemy
{
	GENERATED_BODY()

public:
	AEliteGiant();

	virtual void PerformPrimaryAttack() override;
};
