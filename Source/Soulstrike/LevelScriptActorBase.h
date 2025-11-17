// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "GameFramework/Character.h"
#include "LevelScriptActorBase.generated.h"

/**
 * 
 */
UCLASS()
class SOULSTRIKE_API ALevelScriptActorBase : public ALevelScriptActor
{
	GENERATED_BODY()
	
public:
	void SpawnChests(int32 ChestCount);

protected:
	void BeginPlay() override;

	UPROPERTY()
	TSubclassOf<AActor> ChestActorClass;
};