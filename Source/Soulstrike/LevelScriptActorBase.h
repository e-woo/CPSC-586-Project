// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>

#include "CharacterBase.h"

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

private:
	template <typename T>
	void LoadClass(const std::string& Path, TSubclassOf<T>& SubClass);
};