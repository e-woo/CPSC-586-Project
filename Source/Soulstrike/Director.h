// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>

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
	void LoadEliteClasses();

	void ReceiveSpawnCredits();
	void SpawnSwarmEnemies();
	void SpawnEliteEnemies();

	int CountActors(UClass* Class);
	FVector ChooseEnemySpawnLocation(FVector Origin, float Radius, float MinDistance);

	TWeakObjectPtr<ACharacterBase> PlayerCharacter;

	UPROPERTY()
	TSubclassOf<AActor> EnemyActorClass;

	UPROPERTY()
	TSubclassOf<AActor> EliteArcherActorClass;

	UPROPERTY()
	TSubclassOf<AActor> EliteAssassinActorClass;

	UPROPERTY()
	TSubclassOf<AActor> EliteGiantActorClass;

	UPROPERTY()
	TSubclassOf<AActor> EliteHealerActorClass;

	UPROPERTY()
	TSubclassOf<AActor> ElitePaladinActorClass;

	struct FEliteClassEntry
	{
		const std::string Name;
		TSubclassOf<AActor>* ClassPtr;
	};

	TArray<FEliteClassEntry> EliteClassMap = {
		{ "BP_EliteArcher",   &EliteArcherActorClass },
		{ "BP_EliteAssassin", &EliteAssassinActorClass },
		{ "BP_EliteGiant",    &EliteGiantActorClass },
		{ "BP_EliteHealer",   &EliteHealerActorClass },
		{ "BP_ElitePaladin",  &ElitePaladinActorClass }
	};
	TArray<TSubclassOf<AActor>> EliteClasses;

	double StartTime;
	int32 TickNum = 0;
	int32 SpawnChanceBonus = 0;
	int32 SpawnEliteChanceBonus = 0;

	int32 SpawnCredits = 50;
	int32 BaseCreditAmountToReceive = 10;

	int32 SwarmPackNum = 0;

	// Enemy spawn costs
	int32 EnemySpawnCost = 20;
	int32 EliteSpawnCost = 100;
};
