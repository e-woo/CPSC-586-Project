#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DirectorAI.generated.h"

/**
 * Delegate for when Elite deals damage to player
 * Blueprint can bind to this event
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamagePlayerEvent,
	ACharacter*, TargetPlayer,
	float, Damage,
	AActor*, DamageSource);

/**
 * Director AI - Global manager that tracks the player and broadcasts information to all enemies.
 * Also handles combat events like damage reporting.
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

	/** Called by Elites when they deal damage to the player */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ReportDamageToPlayer(ACharacter* TargetPlayer, float Damage, AActor* DamageSource);

	/** Event that Blueprint can bind to - fires when player takes damage */
	UPROPERTY(BlueprintAssignable, Category = "Combat Events")
	FOnDamagePlayerEvent OnDamagePlayerEvent;

private:
	/** Cached reference to the player character */
	ACharacter* PlayerCharacter;

	/** Last broadcast player position */
	FVector LastPlayerPosition;
};
