#include "SoulstrikeGameMode.h"
#include "EnemyLogicManager.h"
#include "WeightManager.h"

ASoulstrikeGameMode::ASoulstrikeGameMode()
{
	EnemyLogicManager = nullptr;
}

void ASoulstrikeGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Reset Elite AI weights for new game (elites must learn from scratch)
	UWeightManager* WeightMgr = UWeightManager::Get(GetWorld());
	if (WeightMgr)
	{
		WeightMgr->ResetAllWeights();
		UE_LOG(LogTemp, Log, TEXT("SoulstrikeGameMode: Elite AI weights reset for new game. Elites will learn from scratch."));
	}

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
