#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SoulstrikeGameInstance.generated.h"

/**
 * Delegate for broadcasting player position updates
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerPositionUpdated, const FVector&, NewPlayerPosition);

/**
 * Custom Game Instance for Soulstrike
 * Manages global game state and communication between systems
 */
UCLASS()
class SOULSTRIKE_API USoulstrikeGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Delegate broadcast when player position changes */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerPositionUpdated OnPlayerPositionUpdated;
};
