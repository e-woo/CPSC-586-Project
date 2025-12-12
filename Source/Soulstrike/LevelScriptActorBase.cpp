// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelScriptActorBase.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "Landscape.h"
#include "UObject/ConstructorHelpers.h"

#include "Util/LoadBP.h"
#include "Util/Spawn.h"

void ALevelScriptActorBase::BeginPlay()
{
	Super::BeginPlay();

	LoadBP::LoadClass("/Game/Interaction/BP_Chest.BP_Chest_C", ChestActorClass);

	if (ChestActorClass)
	{
		SpawnChests(30);
	}
}

void ALevelScriptActorBase::SpawnChests(int32 ChestCount)
{
	TArray<AActor*> LandscapeActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALandscape::StaticClass(), LandscapeActors);

	if (LandscapeActors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Landscape actors found in the level."));
		return;
	}

	ALandscape* Landscape = Cast<ALandscape>(LandscapeActors[0]);
	if (!Landscape)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast to ALandscape."));
		return;
	}

	FVector Origin, Extent;
	Landscape->GetActorBounds(false, Origin, Extent);

	UWorld* World = GetWorld();
	for (int i = 0; i < ChestCount; i++)
	{
		FActorSpawnParameters SpawnParams;

		AActor* NewChest = Spawn::SpawnActor(World, ChestActorClass, Origin, Extent, 4500.f, 30.f, true, SpawnParams);
		if (NewChest)
		{
#if WITH_EDITOR
			NewChest->SetFolderPath("/Chests");
#endif
		}
	}
}
