// Fill out your copyright notice in the Description page of Project Settings.

#include "Spawn.h"

#include "LevelScriptActorBase.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "Landscape.h"
#include "UObject/ConstructorHelpers.h"

TSubclassOf<AActor> ChestActorClass;

TSubclassOf<AActor> EnemyActorClass;

void ALevelScriptActorBase::BeginPlay()
{
	Super::BeginPlay();

	LoadClass("/Game/Interaction/BP_Chest.BP_Chest_C", ChestActorClass);
	LoadClass("/Game/Enemy/BP_Enemy.BP_Enemy_C", EnemyActorClass);

	if (ChestActorClass)
	{
		SpawnChests(30);
	}
}

template <typename T>
void ALevelScriptActorBase::LoadClass(const std::string& Path, TSubclassOf<T>& SubClass)
{
	static_assert(std::is_base_of<UObject, T>::value);
	FString LPath(Path.c_str());

	UClass* LoadedClass = StaticLoadClass(T::StaticClass(), nullptr, *LPath);
	SubClass = LoadedClass;

	if (!SubClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load class from path: %s"), *LPath);
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

		AActor* NewChest = Spawn::SpawnActor(World, ChestActorClass, Origin, Extent, 12000.f, 30.f, SpawnParams);
		if (NewChest)
		{
			NewChest->SetFolderPath("/Chests");
		}
	}
}
