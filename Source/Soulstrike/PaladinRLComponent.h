#pragma once

#include "CoreMinimal.h"
#include "RLComponent.h"
#include "PaladinRLComponent.generated.h"

/**
 * RL Component for Paladin - Rewards protecting the Healer
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SOULSTRIKE_API UPaladinRLComponent : public URLComponent
{
	GENERATED_BODY()

protected:
	virtual float CalculateReward() override;
};
