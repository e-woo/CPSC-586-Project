// Fill out your copyright notice in the Description page of Project Settings.


#include "Director.h"
#include "Engine.h"
#include <Kismet/GameplayStatics.h>

#include "EliteEnemy.h"
#include "Util/Spawn.h"
#include "Util/LoadBP.h"


// Sets default values
ADirector::ADirector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ADirector::BeginPlay()
{
	Super::BeginPlay();

	PlayerCharacter = Cast<ACharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!PlayerCharacter.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Player character not found in LevelScriptActorBase."));
	}

	LoadBP::LoadClass("/Game/Enemy/BP_Enemy.BP_Enemy_C", EnemyActorClass);

	LoadEliteClasses();

	FTimerHandle DirectorTimerHandle;
	FTimerDelegate DirectorDelegate;

	DirectorDelegate.BindUObject(this, &ADirector::TickDirector);
	GetWorldTimerManager().SetTimer(DirectorTimerHandle, DirectorDelegate, 1.0f, true);

	StartTime = FPlatformTime::Seconds();
}

void ADirector::LoadEliteClasses()
{
	auto LoadEliteClass = [](std::string BpClass, TSubclassOf<AActor>& Class) {
		LoadBP::LoadClass("/Game/ThirdPersonBP/Blueprints/EliteAI/" + BpClass + "." + BpClass + "_C", Class);
	};

	for (auto& Entry : EliteClassMap)
	{
		LoadEliteClass(Entry.Name, *Entry.ClassPtr);

		if (*Entry.ClassPtr)       // Only add valid classes
			EliteClasses.Add(*Entry.ClassPtr);
	}
}

// Called every frame
void ADirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADirector::TickDirector()
{
	ReceiveSpawnCredits();

	// Attempt to spawn enemies every 10 seconds
	if (TickNum >= 10)
	{
		if (FMath::RandRange(1, 100) <= 50 + SpawnChanceBonus)
		{
			if (SpawnCredits >= EliteSpawnCost && FMath::RandRange(1, 100) <= 25 + SpawnEliteChanceBonus)
			{
				SpawnEliteEnemies();
				SpawnEliteChanceBonus = 0;
			}
			else
			{
				SpawnEliteChanceBonus += 25;
				SpawnSwarmEnemies();
			}
			SpawnChanceBonus = 0;
		}
		else
		{
			// Each failed attempt will increase the chance of spawning next time by 25%
			SpawnChanceBonus += 25;
		}

		TickNum = 0;
	}

	UE_LOG(LogTemp, Display, TEXT("Current credit count: %d"), SpawnCredits);
	TickNum += 1;
}

void ADirector::ReceiveSpawnCredits()
{
	float Multiplier = 1.f;

	Multiplier += (FPlatformTime::Seconds() - StartTime) / 60.f;
	Multiplier *= PlayerCharacter->CurrentHP / PlayerCharacter->MaxHP;

	Multiplier *= (1 - 0.16f * CountActors(AEliteEnemy::StaticClass()));

	Multiplier *= FMath::Min(1.f, 20.f / (CountActors(EnemyActorClass) + 10));
	UE_LOG(LogTemp, Display, TEXT("Current credit multiplier: %f"), Multiplier);

	SpawnCredits += FMath::RoundToInt(BaseCreditAmountToReceive * Multiplier);
}

void ADirector::SpawnSwarmEnemies()
{
	// Pack size: minimum 3, maximum 10
	int EnemiesToSpawn = FMath::RandRange(3, 10);
	EnemiesToSpawn = FMath::Min(EnemiesToSpawn, SpawnCredits / EnemySpawnCost);

	UE_LOG(LogTemp, Display, TEXT("Spawning %d enemies."), EnemiesToSpawn);
	float SpawnRadius = 6000.f;
	FVector PlayerLocation = PlayerCharacter->GetActorLocation();

	FVector SpawnLocation = ChooseEnemySpawnLocation(PlayerLocation, SpawnRadius, 3000.f);

	UWorld* World = GetWorld();
	FName Path = FName("Enemies/SwarmEnemies/Pack_" + FString::FromInt(SwarmPackNum++));
	for (int i = 0; i < EnemiesToSpawn; i++)
	{
		FActorSpawnParameters SpawnParams;

		AActor* NewEnemy = Spawn::SpawnActor(World, EnemyActorClass, SpawnLocation, FVector(500.f, 500.f, 10000.f), 0, 60.f, false, SpawnParams, FVector(0, 0, 100));
		if (NewEnemy)
		{
			NewEnemy->SetFolderPath(Path);
		}
	}
	SpawnCredits -= EnemiesToSpawn * EnemySpawnCost;
}

void ADirector::SpawnEliteEnemies()
{
	int EnemiesToSpawn = FMath::RandRange(1, 2);
	EnemiesToSpawn = FMath::Min(EnemiesToSpawn, SpawnCredits / EliteSpawnCost);

	for (int i = 0; i < EnemiesToSpawn; i++)
	{
		TSubclassOf<AActor> SelectedEliteClass = EliteClasses[FMath::RandRange(0, EliteClasses.Num() - 1)];
		FVector PlayerLocation = PlayerCharacter->GetActorLocation();
		FVector SpawnLocation = ChooseEnemySpawnLocation(PlayerLocation, 3000.f, 4000.f);

		UWorld* World = GetWorld();

		FActorSpawnParameters SpawnParams;
		AActor* NewElite = Spawn::SpawnActor(World, SelectedEliteClass, SpawnLocation, FVector(500.f, 500.f, 10000.f), 0, 60.f, false, SpawnParams, FVector(0, 0, 100));
		if (NewElite)
		{
			NewElite->SetFolderPath("Enemies/EliteEnemies");
		}
	}
	SpawnCredits -= EnemiesToSpawn * EliteSpawnCost;
}

FVector ADirector::ChooseEnemySpawnLocation(FVector Origin, float Radius, float MinDistance)
{
	int MaxSpawnAttempts = 100;

	UWorld* World = GetWorld();

	for (int i = 0; i < MaxSpawnAttempts; i++)
	{
		float RandomX = FMath::RandRange(Origin.X - Radius, Origin.X + Radius);
		float RandomY = FMath::RandRange(Origin.Y - Radius, Origin.Y + Radius);

		if (FVector::Dist2D(FVector(RandomX, RandomY, 0.f), Origin) < MinDistance)
		{
			continue;
		}

		FVector High = FVector(RandomX, RandomY, Origin.Z + 10000.f);
		FVector Low = FVector(RandomX, RandomY, 0);

		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.bTraceComplex = true;

		if (World->LineTraceSingleByChannel(HitResult, High, Low, ECC_Visibility, Params))
		{
			return HitResult.Location;
		}
	}

	return FVector(0, 0, 0);
}

int ADirector::CountActors(UClass* Class)
{
	int Count = 0;
	UWorld* World = GetWorld();
	for (TActorIterator<AActor> It(World, Class); It; ++It)
	{
		Count++;
	}
	UE_LOG(LogTemp, Display, TEXT("Counted %d actors of class %s"), Count, *Class->GetName());
	return Count;
}