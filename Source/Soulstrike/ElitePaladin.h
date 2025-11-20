#pragma once

#include "CoreMinimal.h"
#include "EliteEnemy.h"
#include "ElitePaladin.generated.h"

/**
 * Elite Paladin - Tank that protects the Healer
 * Has regeneration and stays close to allies
 */
UCLASS()
class SOULSTRIKE_API AElitePaladin : public AEliteEnemy
{
	GENERATED_BODY()

public:
	AElitePaladin();
};
