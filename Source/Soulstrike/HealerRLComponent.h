#pragma once

#include "CoreMinimal.h"
#include "RLComponent.h"
#include "HealerRLComponent.generated.h"

/**
 * RL Component for Healer - Rewards healing allies and staying safe
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SOULSTRIKE_API UHealerRLComponent : public URLComponent
{
	GENERATED_BODY()

protected:
	virtual float CalculateReward() override;
	virtual void PerformSecondaryAttackOnElite() override;
};
