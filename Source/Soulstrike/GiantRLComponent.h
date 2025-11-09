#pragma once

#include "CoreMinimal.h"
#include "RLComponent.h"
#include "GiantRLComponent.generated.h"

/**
 * RL Component for Giant - Rewards tanking and staying alive
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SOULSTRIKE_API UGiantRLComponent : public URLComponent
{
	GENERATED_BODY()

protected:
	virtual float CalculateReward() override;
};
