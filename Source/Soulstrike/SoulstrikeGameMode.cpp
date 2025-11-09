#include "SoulstrikeGameMode.h"
#include "DirectorAI.h"

ASoulstrikeGameMode::ASoulstrikeGameMode()
{
	DirectorAI = nullptr;
}

void ASoulstrikeGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Spawn Director AI
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = FName(TEXT("DirectorAI"));
	SpawnParams.Owner = this;

	DirectorAI = GetWorld()->SpawnActor<ADirectorAI>(ADirectorAI::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (DirectorAI)
	{
		UE_LOG(LogTemp, Log, TEXT("SoulstrikeGameMode: Director AI spawned successfully."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SoulstrikeGameMode: Failed to spawn Director AI!"));
	}
}
