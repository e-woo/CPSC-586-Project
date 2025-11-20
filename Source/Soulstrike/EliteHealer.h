#pragma once

#include "CoreMinimal.h"
#include "EliteEnemy.h"
#include "EliteHealer.generated.h"

/**
 * Elite Healer - Support class
 * Heals allies, stays at range
 */
UCLASS()
class SOULSTRIKE_API AEliteHealer : public AEliteEnemy
{
	GENERATED_BODY()

public:
	AEliteHealer();
};
