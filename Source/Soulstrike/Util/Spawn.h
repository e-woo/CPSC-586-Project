// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "CoreMinimal.h"
/**
 * 
 */
class SOULSTRIKE_API Spawn
{
public:
	Spawn() = delete;

	static AActor* SpawnActor(UWorld* World, UClass* Class, const FVector Origin, const FVector Extent, const float MinDistanceFromOrigin, const float MaxSlopeAngle, const bool Rotate, const FActorSpawnParameters& SpawnParams);
	static AActor* SpawnActor(UWorld* World, UClass* Class, const FVector Origin, const FVector Extent, const float MinDistanceFromOrigin, const float MaxSlopeAngle, const bool Rotate, const FActorSpawnParameters& SpawnParams, FVector SpawnOffset);

private:
	static FVector GetGroundLocationAndNormal(UWorld* World, FVector Origin, FVector Extent, float MinDistanceFromOrigin, float MaxSlopeAngle, FRotator& Rotation);
	static float GetSlopeAngleDegrees(const FVector& Normal);

};
