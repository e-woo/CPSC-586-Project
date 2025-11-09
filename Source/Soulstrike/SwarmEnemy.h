#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SwarmEnemy.generated.h"

UCLASS()
class SOULSTRIKE_API ASwarmEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASwarmEnemy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Current health of the swarm enemy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm Enemy|Health")
	float Health;

	/** Maximum health of the swarm enemy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm Enemy|Health")
	float MaxHealth;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Apply damage to the swarm enemy */
	UFUNCTION(BlueprintCallable, Category = "Swarm Enemy|Health")
	void TakeDamageCustom(float DamageAmount);

	/** Check if the swarm enemy is alive */
	UFUNCTION(BlueprintPure, Category = "Swarm Enemy|Health")
	bool IsAlive() const;

	/** Get current health percentage (0.0 to 1.0) */
	UFUNCTION(BlueprintPure, Category = "Swarm Enemy|Health")
	float GetHealthPercent() const;
};