// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CharacterBase.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Director.generated.h"

UCLASS(Blueprintable)
class SOULSTRIKE_API ADirector : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADirector();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void TickDirector();

private:

	void ReceiveSpawnCredits();
	void SpawnEnemies();
	FVector ChooseEnemySpawnLocation(FVector Origin, float Radius, float MinDistance);

	ACharacterBase* PlayerCharacter;

	double StartTime;
	int32 SpawnCredits = 10;
	int32 BaseCreditAmountToReceive = 10;
};
