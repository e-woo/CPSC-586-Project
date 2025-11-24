#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SwarmAIController.generated.h"

/**
 * AI Controller for swarm enemies
 * Handles possession and basic AI logic for ASwarmEnemy
 */
UCLASS()
class SOULSTRIKE_API ASwarmAIController : public AAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this controller's properties
	ASwarmAIController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called when the controller possesses a pawn
	virtual void OnPossess(APawn* InPawn) override;

	// Called when the controller unpossesses a pawn
	virtual void OnUnPossess() override;

private:
	TMap<FString, TArray<TWeakObjectPtr<APawn>>> SwarmMap;

public:
	// ========== Boids Configuration Properties ==========

	/** How far the Boid can "see" its neighbors */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Perception")
	float PerceptionRadius;

	/** Multiplier for the separation force */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Weights")
	float SeparationWeight;

	/** Multiplier for the alignment force */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Weights")
	float AlignmentWeight;

	/** Multiplier for the cohesion force */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Weights")
	float CohesionWeight;

	/** Multiplier for the target attraction force */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Weights")
	float TargetAttractionWeight;

	/** The top speed of the Boid */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Movement")
	float MaxSpeed;

	/** The maximum steering force that can be applied */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Movement")
	float MaxForce;

	/** Target location to move towards (set by Director AI or player tracking) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Target")
	FVector TargetLocation;

	// ========== Debug Visualization ==========

	/** Enable debug visualization of perception radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Debug")
	bool bShowPerceptionRadius;

	/** Enable debug visualization of steering forces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids|Debug")
	bool bShowSteeringForces;

protected:
	// ========== Boids Calculation Functions ==========

	/** Find all nearby swarm enemies within perception radius */
	void GetNearbyBoids(APawn* Target, TArray<TWeakObjectPtr<APawn>>& OutNeighbors);

	/** Calculate separation force (avoid crowding neighbors) */
	FVector CalculateSeparation(APawn* Target, const TArray<TWeakObjectPtr<APawn>>& Neighbors);

	/** Calculate alignment force (match velocity with neighbors) */
	FVector CalculateAlignment(APawn* Target, const TArray<TWeakObjectPtr<APawn>>& Neighbors);

	/** Calculate cohesion force (move towards center of mass) */
	FVector CalculateCohesion(APawn* Target, const TArray<TWeakObjectPtr<APawn>>& Neighbors);

	/** Calculate attraction force towards target location */
	FVector CalculateTargetAttraction(APawn* Target);

	/** Apply the combined steering forces to the character's movement */
	void ApplySteeringForce(APawn* Target, const FVector& SteeringForce, float DeltaTime);

	/** Draw debug visualizations */
	void DrawDebugVisualization(APawn* Target, const FVector& Separation, const FVector& Alignment, const FVector& Cohesion, const FVector& TargetAttraction);
};
