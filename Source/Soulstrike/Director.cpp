// Fill out your copyright notice in the Description page of Project Settings.


#include "Director.h"
#include <Kismet/GameplayStatics.h>

#include "EliteEnemy.h"

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
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player character not found in LevelScriptActorBase."));
	}

	StartTime = FPlatformTime::Seconds();

	FTimerHandle DirectorTimerHandle;
	FTimerDelegate DirectorDelegate;

	DirectorDelegate.BindUObject(this, &ADirector::TickDirector);
	GetWorldTimerManager().SetTimer(DirectorTimerHandle, DirectorDelegate, 1.0f, true);
	
}

// Called every frame
void ADirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

template <typename T>
void ADirector::LoadClass(const std::string& Path, TSubclassOf<T>& SubClass)
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

void ADirector::TickDirector()
{
	UE_LOG(LogTemp, Display, TEXT("TickEvent"));

	ReceiveSpawnCredits();
	if (FMath::RandRange(1, 100) <= 5)
	{
		SpawnEnemies();
		UE_LOG(LogTemp, Display, TEXT("Spawning enemies this tick."));
	}
	UE_LOG(LogTemp, Display, TEXT("Current credit count: %d"), SpawnCredits);
}

void ADirector::ReceiveSpawnCredits()
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

void ADirector::SpawnEnemies()
{
	if (FMath::RandRange(1, 100) > 5)
	{
		return;
	}

	int EnemiesToSpawn = FMath::RandRange(1, SpawnCredits);
	float SpawnRadius = 6000.f;
	FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	FVector Extent = FVector(SpawnRadius, SpawnRadius, 0.f);

	float RandomX = FMath::RandRange(PlayerLocation.X - Extent.X, PlayerLocation.X + Extent.X);
	float RandomY = FMath::RandRange(PlayerLocation.Y - Extent.Y, PlayerLocation.Y + Extent.Y);
	//for (int i = 0; i < EnemiesToSpawn; i++)
	//{
	//	FRotator SpawnRotation;
	//	FVector SpawnLocation = GetGroundLocationAndNormal(PlayerLocation, Extent, 3000.f, 30.f, SpawnRotation);
	//	FActorSpawnParameters SpawnParams;

	//	AActor* NewEnemy = GetWorld()->SpawnActor<AActor>(ChestActorClass, SpawnLocation, SpawnRotation, SpawnParams);
	//	if (NewEnemy)
	//	{
	//		NewEnemy->SetFolderPath("/SwarmEnemies");
	//	}
	//	else
	//	{
	//		UE_LOG(LogTemp, Warning, TEXT("Attempted to spawn enemy at %s but failed."), *SpawnLocation.ToString());
	//	}
	//}
}