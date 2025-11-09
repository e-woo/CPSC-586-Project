#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DirectorAI.generated.h"

/**
 * Director AI - Global manager that tracks the player and broadcasts information to all enemies.
 * This is a pure logic actor with no physical representation.
 */
UCLASS(NotPlaceable, NotBlueprintable)
class SOULSTRIKE_API ADirectorAI : public AActor
{
	GENERATED_BODY()
	
public:	
	ADirectorAI();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

private:
	/** Cached reference to the player character */
	ACharacter* PlayerCharacter;

	/** Last broadcast player position */
	FVector LastPlayerPosition;
};
