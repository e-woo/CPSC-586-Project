#pragma once

#include "CoreMinimal.h"
#include "EliteEnemy.h"
#include "ElitePaladin.generated.h"

class ADirectorAI;

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

protected:
	virtual void BeginPlay() override;

public:
	virtual void PerformPrimaryAttack() override;

private:
	/** Cached reference to DirectorAI to avoid repeated lookups */
	UPROPERTY()
	ADirectorAI* CachedDirector;
};
