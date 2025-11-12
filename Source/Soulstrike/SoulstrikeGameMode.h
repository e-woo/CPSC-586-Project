#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SoulstrikeGameMode.generated.h"

class AEnemyLogicManager;

/**
 * Main Game Mode for Soulstrike
 * Handles Enemy Logic Manager spawning and game logic
 */
UCLASS()
class SOULSTRIKE_API ASoulstrikeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASoulstrikeGameMode();

protected:
	virtual void BeginPlay() override;

private:
	/** Reference to the spawned Enemy Logic Manager */
	UPROPERTY()
	AEnemyLogicManager* EnemyLogicManager;
};
