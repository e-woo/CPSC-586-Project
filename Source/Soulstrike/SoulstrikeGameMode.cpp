#include "SoulstrikeGameMode.h"
#include "EnemyLogicManager.h"

ASoulstrikeGameMode::ASoulstrikeGameMode()
{
	EnemyLogicManager = nullptr;
}

void ASoulstrikeGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Spawn Enemy Logic Manager
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = FName(TEXT("EnemyLogicManager"));
	SpawnParams.Owner = this;

	EnemyLogicManager = GetWorld()->SpawnActor<AEnemyLogicManager>(AEnemyLogicManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (EnemyLogicManager)
	{
		UE_LOG(LogTemp, Log, TEXT("SoulstrikeGameMode: Enemy Logic Manager spawned successfully."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SoulstrikeGameMode: Failed to spawn Enemy Logic Manager!"));
	}
}
