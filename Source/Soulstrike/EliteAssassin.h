#pragma once

#include "CoreMinimal.h"
#include "EliteEnemy.h"
#include "EliteAssassin.generated.h"

/**
 * Elite Assassin - Fast melee with poison
 * Applies damage over time
 */
UCLASS()
class SOULSTRIKE_API AEliteAssassin : public AEliteEnemy
{
	GENERATED_BODY()

public:
	AEliteAssassin();
};
