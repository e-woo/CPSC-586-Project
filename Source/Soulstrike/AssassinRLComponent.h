#pragma once

#include "CoreMinimal.h"
#include "RLComponent.h"
#include "AssassinRLComponent.generated.h"

/**
 * RL Component for Assassin - Rewards close-mid range combat
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SOULSTRIKE_API UAssassinRLComponent : public URLComponent
{
	GENERATED_BODY()

protected:
	virtual float CalculateReward() override;
};
