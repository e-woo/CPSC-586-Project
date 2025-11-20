#pragma once

#include "CoreMinimal.h"
#include "EliteEnemy.h"
#include "EliteGiant.generated.h"

/**
 * Elite Giant - Slow but powerful
 * High damage, long cooldown
 */
UCLASS()
class SOULSTRIKE_API AEliteGiant : public AEliteEnemy
{
	GENERATED_BODY()

public:
	AEliteGiant();
};
