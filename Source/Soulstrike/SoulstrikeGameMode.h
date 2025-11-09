#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SoulstrikeGameMode.generated.h"

class ADirectorAI;

/**
 * Main Game Mode for Soulstrike
 * Handles Director AI spawning and game logic
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
	/** Reference to the spawned Director AI */
	UPROPERTY()
	ADirectorAI* DirectorAI;
};
