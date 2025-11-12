// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelScriptActorBase.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "Landscape.h"
#include "UObject/ConstructorHelpers.h"

#include "EliteEnemy.h"

TSubclassOf<AActor> ChestActorClass;

void ALevelScriptActorBase::BeginPlay()
{
	Super::BeginPlay();
	FString Path = TEXT("/Game/Interaction/BP_Chest.BP_Chest_C");
	ChestActorClass = Cast<UClass>(StaticLoadClass(AActor::StaticClass(), nullptr, *Path));

	if (ChestActorClass)
	{
		SpawnChests(30);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load Chest actor class from path: %s"), *Path);
	}

	FTimerHandle DirectorTimerHandle;
	FTimerDelegate DirectorDelegate;

	DirectorDelegate.BindUObject(this, &ALevelScriptActorBase::TickDirector);
	GetWorldTimerManager().SetTimer(DirectorTimerHandle, DirectorDelegate, 1.0f, true);

	PlayerCharacter = Cast<ACharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player character not found in LevelScriptActorBase."));
	}

	StartTime = FPlatformTime::Seconds();
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

	for (int i = 0; i < ChestCount; i++)
	{
		FRotator SpawnRotation;
		FVector SpawnLocation = GetGroundLocationAndNormal(Origin, Extent, SpawnRotation);
		FActorSpawnParameters SpawnParams;

		//UE_LOG(LogTemp, Display, TEXT("Spawning chest at location: %s"), *SpawnLocation.ToString());

		AActor* NewChest = GetWorld()->SpawnActor<AActor>(ChestActorClass, SpawnLocation, SpawnRotation, SpawnParams);
		if (NewChest)
		{
			NewChest->SetFolderPath("/Chests");
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Attempted to spawn chest at %s but failed."), *SpawnLocation.ToString());
		}
	}
}

FVector ALevelScriptActorBase::GetGroundLocationAndNormal(FVector Origin, FVector Extent, FRotator& Rotation)
{
	int MaxSpawnAttempts = 100;
	float MinDistanceFromOrigin = 12000.f;
	float MaxSlopeAngle = 30.f;

	for (int i = 0; i < MaxSpawnAttempts; i++)
	{
		float RandomX = FMath::RandRange(Origin.X - Extent.X, Origin.X + Extent.X);
		float RandomY = FMath::RandRange(Origin.Y - Extent.Y, Origin.Y + Extent.Y);

		if (FVector::Dist2D(FVector(RandomX, RandomY, 0.f), Origin) < MinDistanceFromOrigin)
		{
			continue;
		}

		FVector High = FVector(RandomX, RandomY, Origin.Z + Extent.Z);
		FVector Low = FVector(RandomX, RandomY, Origin.Z - Extent.Z);

		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.bTraceComplex = true;

		if (GetWorld()->LineTraceSingleByChannel(HitResult, High, Low, ECC_Visibility, Params))
		{
			FVector Normal = HitResult.Normal;

			float SlopeAngle = GetSlopeAngleDegrees(Normal);
			if (SlopeAngle > MaxSlopeAngle)  // Too steep
			{
				continue;
			}

			Rotation = FRotationMatrix::MakeFromXZ(FVector::ForwardVector, Normal).Rotator();
			Rotation.Yaw += FMath::FRandRange(0.f, 360.f);

			return HitResult.Location;
		}
	}

	// Fallback
	Rotation = FRotator::ZeroRotator;
    return FVector(0, 0, 0.f);
}

float ALevelScriptActorBase::GetSlopeAngleDegrees(const FVector& Normal)
{
	float CosTheta = FVector::DotProduct(Normal, FVector::UpVector);
	CosTheta = FMath::Clamp(CosTheta, -1.f, 1.f); // avoid NaN
	return FMath::Acos(CosTheta) * (180.f / PI);
}

void ALevelScriptActorBase::TickDirector()
{
	UE_LOG(LogTemp, Display, TEXT("TickEvent"));

	ReceiveSpawnCredits();
	SpawnEnemies();
	UE_LOG(LogTemp, Display, TEXT("Current credit count: %d"), SpawnCredits);
}

void ALevelScriptActorBase::ReceiveSpawnCredits()
{
	float Multiplier = 1.f;

	Multiplier += (FPlatformTime::Seconds() - StartTime) / 60.f;
	Multiplier *= PlayerCharacter->CurrentHP / PlayerCharacter->MaxHP;

	TArray<AActor*> EliteEnemyActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEliteEnemy::StaticClass(), EliteEnemyActors);

	Multiplier *= (1 - 0.16f * EliteEnemyActors.Num());

	//UE_LOG(LogTemp, Display, TEXT("Current credit multiplier: %f"), Multiplier);

	SpawnCredits += FMath::RoundToInt(BaseCreditAmountToReceive * Multiplier);
}

void ALevelScriptActorBase::SpawnEnemies()
{
	// TODO: Enemy spawn logic
}