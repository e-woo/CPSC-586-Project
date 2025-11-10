// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelScriptActorBase.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "Landscape.h"
#include "UObject/ConstructorHelpers.h"

TSubclassOf<AActor> ChestActorClass;

void ALevelScriptActorBase::BeginPlay()
{
	Super::BeginPlay();
	FString Path = TEXT("/Game/Interaction/BP_Chest.BP_Chest_C");
	ChestActorClass = Cast<UClass>(StaticLoadClass(AActor::StaticClass(), nullptr, *Path));

	if (ChestActorClass)
	{
		SpawnChests(50);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load Chest actor class from path: %s"), *Path);
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

	float MinDistanceFromOrigin = 8000.f;
	int maxSpawnAttempts = 100;
	for (int i = 0; i < ChestCount; i++)
	{
		for (int j = 0; j < maxSpawnAttempts; j++)
		{
			float RandomX = FMath::RandRange(Origin.X - Extent.X, Origin.X + Extent.X);
			float RandomY = FMath::RandRange(Origin.Y - Extent.Y, Origin.Y + Extent.Y);
			float RandomZ = FMath::RandRange(Origin.Z - Extent.Z, Origin.Z + Extent.Z);

			if (FVector::Dist2D(FVector(RandomX, RandomY, 0.f), Origin) >= MinDistanceFromOrigin)
			{
				FVector SpawnLocation = GetGroundLocation(RandomX, RandomY, Origin.Z - Extent.Z, Origin.Z + Extent.Z);
				FActorSpawnParameters SpawnParams;

				//UE_LOG(LogTemp, Display, TEXT("Spawning chest at location: %s"), *SpawnLocation.ToString());

				AActor* NewChest = GetWorld()->SpawnActor<AActor>(ChestActorClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
				if (NewChest)
				{
					NewChest->SetFolderPath("/Chests");
				}
				break;
			}
		}
	}
}

FVector ALevelScriptActorBase::GetGroundLocation(float X, float Y, float MinZ, float MaxZ)
{
	FVector High = FVector(X, Y, MaxZ);
	FVector Low = FVector(X, Y, MinZ);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.bTraceComplex = true;

	if (GetWorld()->LineTraceSingleByChannel(HitResult, High, Low, ECC_Visibility, Params))
	{
		return HitResult.Location;
	}
    return FVector(X, Y, 0.f);
}