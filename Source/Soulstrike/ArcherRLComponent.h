#pragma once

#include "CoreMinimal.h"
#include "RLComponent.h"
#include "ArcherRLComponent.generated.h"

/**
 * RL Component for Archer - Rewards maintaining distance
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SOULSTRIKE_API UArcherRLComponent : public URLComponent
{
	GENERATED_BODY()

protected:
	virtual float CalculateReward() override;
};
