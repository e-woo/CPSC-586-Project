#include "DirectorAI.h"
#include "SoulstrikeGameInstance.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

ADirectorAI::ADirectorAI()
{
	PrimaryActorTick.bCanEverTick = true;

	// No physical representation
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ADirectorAI::BeginPlay()
{
	Super::BeginPlay();

	// Get player reference
	PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	if (PlayerCharacter)
	{
		LastPlayerPosition = PlayerCharacter->GetActorLocation();
		UE_LOG(LogTemp, Log, TEXT("DirectorAI: Player character found and tracked."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DirectorAI: Player character not found!"));
	}
}

void ADirectorAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!PlayerCharacter)
	{
		// Try to find player again
		PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
		return;
	}

	// Get current player position
	FVector CurrentPlayerPosition = PlayerCharacter->GetActorLocation();

	// Broadcast player position via GameInstance delegate
	USoulstrikeGameInstance* GameInstance = Cast<USoulstrikeGameInstance>(GetWorld()->GetGameInstance());
	if (GameInstance)
	{
		GameInstance->OnPlayerPositionUpdated.Broadcast(CurrentPlayerPosition);
	}

	LastPlayerPosition = CurrentPlayerPosition;
}
