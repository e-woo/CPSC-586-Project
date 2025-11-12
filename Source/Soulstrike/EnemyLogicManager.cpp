#include "EnemyLogicManager.h"
#include "SoulstrikeGameInstance.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

AEnemyLogicManager::AEnemyLogicManager()
{
	PrimaryActorTick.bCanEverTick = true;

	// No physical representation
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void AEnemyLogicManager::BeginPlay()
{
	Super::BeginPlay();

	// Get player reference
	PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	if (PlayerCharacter)
	{
		LastPlayerPosition = PlayerCharacter->GetActorLocation();
		UE_LOG(LogTemp, Log, TEXT("EnemyLogicManager: Player character found and tracked."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyLogicManager: Player character not found!"));
	}
}

void AEnemyLogicManager::Tick(float DeltaTime)
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

void AEnemyLogicManager::ReportDamageToPlayer(ACharacter* TargetPlayer, float Damage, AActor* DamageSource)
{
	if (!TargetPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyLogicManager: Attempted to damage null player!"));
		return;
	}

	// Log damage for debugging
	FString SourceName = DamageSource ? DamageSource->GetName() : TEXT("Unknown");
	UE_LOG(LogTemp, Log, TEXT("EnemyLogicManager: %s dealt %.1f damage to player"), *SourceName, Damage);

	// Broadcast event that Blueprint can listen to
	OnDamagePlayerEvent.Broadcast(TargetPlayer, Damage, DamageSource);
}
