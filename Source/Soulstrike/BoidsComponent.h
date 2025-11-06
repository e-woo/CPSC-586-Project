#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BoidsComponent.generated.h"

/**
 * Component that handles Boids flocking behavior
 * Implements Separation, Alignment, and Cohesion rules
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SOULSTRIKE_API UBoidsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBoidsComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

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
	void GetNearbyBoids(TArray<class ASwarmEnemy*>& OutNeighbors);

	/** Calculate separation force (avoid crowding neighbors) */
	FVector CalculateSeparation(const TArray<class ASwarmEnemy*>& Neighbors);

	/** Calculate alignment force (match velocity with neighbors) */
	FVector CalculateAlignment(const TArray<class ASwarmEnemy*>& Neighbors);

	/** Calculate cohesion force (move towards center of mass) */
	FVector CalculateCohesion(const TArray<class ASwarmEnemy*>& Neighbors);

	/** Calculate attraction force towards target location */
	FVector CalculateTargetAttraction();

	/** Apply the combined steering forces to the character's movement */
	void ApplySteeringForce(const FVector& SteeringForce, float DeltaTime);

	/** Draw debug visualizations */
	void DrawDebugVisualization(const FVector& Separation, const FVector& Alignment, const FVector& Cohesion, const FVector& TargetAttraction);

private:
	/** Cached reference to the owning swarm enemy */
	UPROPERTY()
	class ASwarmEnemy* OwnerEnemy;

	/** Cached reference to the character movement component */
	UPROPERTY()
	class UCharacterMovementComponent* MovementComponent;
};
