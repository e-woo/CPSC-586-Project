// Fill out your copyright notice in the Description page of Project Settings.


#include "Director.h"
#include "Engine.h"
#include <Kismet/GameplayStatics.h>

#include "EliteEnemy.h"
#include "Util/Spawn.h"
#include "Util/LoadBP.h"
#include "SwarmAIController.h"


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

	LoadBP::LoadClass("/Game/ThirdPersonBP/Blueprints/SwarmAI/BP_SwarmEnemy.BP_SwarmEnemy_C", EnemyActorClass);
	
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
	int BaseChance = 50 + PlayerCharacter->Level;

	// Attempt to spawn enemies every 6 seconds
	if (TickNum >= 6)
	{
		if (FMath::RandRange(1, 100) <= BaseChance + SpawnChanceBonus)
		{
			if (SpawnCredits >= EliteSpawnCost && FMath::RandRange(1, 100) <= 50 + SpawnEliteChanceBonus)
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

	Multiplier *= FMath::Min(1.f, 20.f / CountActors(EnemyActorClass));
	UE_LOG(LogTemp, Display, TEXT("Current credit multiplier: %f"), Multiplier);

	SpawnCredits += FMath::RoundToInt(BaseCreditAmountToReceive * Multiplier);
}

void ADirector::SpawnSwarmEnemies()
{
	int MaxSwarmEnemyCount = 40;
	if (CountActors(EnemyActorClass) >= MaxSwarmEnemyCount)
	{
		return;
	}

	int MinEnemyCount =
		(PlayerCharacter->Level < 5) ? 3 :
		(PlayerCharacter->Level < 10) ? 6 :
		9;
	int MaxEnemyCount =
		(PlayerCharacter->Level < 5) ? 5 :
		(PlayerCharacter->Level < 10) ? 10 :
		15;

	int EnemiesToSpawn = FMath::RandRange(MinEnemyCount, MaxEnemyCount);
	EnemiesToSpawn = FMath::Min(EnemiesToSpawn, SpawnCredits / EnemySpawnCost);

	UE_LOG(LogTemp, Display, TEXT("Spawning %d enemies."), EnemiesToSpawn);
	float SpawnRadius = 6000.f;
	FVector PlayerLocation = PlayerCharacter->GetActorLocation();

	FVector SpawnLocation = ChooseEnemySpawnLocation(PlayerLocation, SpawnRadius, 3000.f);

	UWorld* World = GetWorld();
	FName Path = FName("Enemies/SwarmEnemies/Pack_" + FString::FromInt(SwarmPackNum++));

	FGuid SwarmId = FGuid::NewGuid();

	for (int i = 0; i < EnemiesToSpawn; i++)
	{
		FActorSpawnParameters SpawnParams;

		AActor* NewEnemy = Spawn::SpawnActor(World, EnemyActorClass, SpawnLocation, FVector(750.f, 750.f, 10000.f), 0, 60.f, false, SpawnParams, FVector(0, 0, 200));
		if (!NewEnemy) continue;
		if (APawn* PawnEnemy = Cast<APawn>(NewEnemy))
		{
#if WITH_EDITOR
			PawnEnemy->SetFolderPath(Path);
#endif
			ASwarmAIController* Controller = Cast<ASwarmAIController>(PawnEnemy->GetController());
			if (Controller)
				Controller->RegisterSwarmEnemy(PawnEnemy, SwarmId);
		}
	}
	SpawnCredits -= EnemiesToSpawn * EnemySpawnCost;
}

void ADirector::SpawnEliteEnemies()
{
	TArray<TSubclassOf<AActor>> AvailableEliteClasses;
	for (auto& EliteClass : EliteClasses)
	{
		if (CountActors(*EliteClass) == 0)
		{
			AvailableEliteClasses.Add(EliteClass);
		}
	}
	if (AvailableEliteClasses.Num() == 0)
	{
		SpawnSwarmEnemies();
		return;
	}

	TSubclassOf<AActor> SelectedEliteClass = AvailableEliteClasses[FMath::RandRange(0, AvailableEliteClasses.Num() - 1)];
	FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	FVector SpawnLocation = ChooseEnemySpawnLocation(PlayerLocation, 3000.f, 2000.f);

	UWorld* World = GetWorld();

	FActorSpawnParameters SpawnParams;
	AActor* NewElite = Spawn::SpawnActor(World, SelectedEliteClass, SpawnLocation, FVector(500.f, 500.f, 10000.f), 0, 60.f, false, SpawnParams, FVector(0, 0, 100));
	if (NewElite)
	{
#if WITH_EDITOR
		NewElite->SetFolderPath("Enemies/EliteEnemies");
#endif
	}
	SpawnCredits -= EliteSpawnCost;
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
	if (!Class)
	{
		return 0;
	}
	int Count = 0;
	UWorld* World = GetWorld();
	for (TActorIterator<AActor> It(World, Class); It; ++It)
	{
		Count++;
	}
	UE_LOG(LogTemp, Display, TEXT("Counted %d actors of class %s"), Count, *Class->GetName());
	return Count;
}