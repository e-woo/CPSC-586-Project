// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
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
	FVector GetGroundLocation(float X, float Y, float MinZ, float MaxZ);
};
