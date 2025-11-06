#include "BoidsComponent.h"
#include "SwarmEnemy.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
UBoidsComponent::UBoidsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame
	PrimaryComponentTick.bCanEverTick = true;

	// Initialize default Boids parameters
	PerceptionRadius = 500.0f;
	SeparationWeight = 1.5f;
	AlignmentWeight = 1.0f;
	CohesionWeight = 1.0f;
	TargetAttractionWeight = 2.0f;
	MaxSpeed = 600.0f;
	MaxForce = 300.0f;

	// Initialize debug flags
	bShowPerceptionRadius = false;
	bShowSteeringForces = false;

	// Initialize cached references
	OwnerEnemy = nullptr;
	MovementComponent = nullptr;
	TargetLocation = FVector::ZeroVector;
}

// Called when the game starts
void UBoidsComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache reference to the owning swarm enemy
	OwnerEnemy = Cast<ASwarmEnemy>(GetOwner());
	
	if (OwnerEnemy)
	{
		// Cache the character movement component
		MovementComponent = OwnerEnemy->GetCharacterMovement();
		
		if (!MovementComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("BoidsComponent: No CharacterMovementComponent found on %s"), *OwnerEnemy->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BoidsComponent: Owner is not a SwarmEnemy!"));
	}
}

// Called every frame
void UBoidsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Early exit if we don't have valid references
	if (!OwnerEnemy || !MovementComponent)
	{
		return;
	}

	// Find nearby neighbors
	TArray<ASwarmEnemy*> Neighbors;
	GetNearbyBoids(Neighbors);

	// Calculate individual steering forces
	FVector Separation = FVector::ZeroVector;
	FVector Alignment = FVector::ZeroVector;
	FVector Cohesion = FVector::ZeroVector;

	// Only calculate flock forces if we have neighbors
	if (Neighbors.Num() > 0)
	{
		Separation = CalculateSeparation(Neighbors) * SeparationWeight;
		Alignment = CalculateAlignment(Neighbors) * AlignmentWeight;
		Cohesion = CalculateCohesion(Neighbors) * CohesionWeight;
	}

	// Calculate target attraction force
	FVector TargetAttraction = CalculateTargetAttraction() * TargetAttractionWeight;

	// Combine all forces
	FVector CombinedForce = Separation + Alignment + Cohesion + TargetAttraction;

	// Apply the steering force
	ApplySteeringForce(CombinedForce, DeltaTime);

	// Draw debug visualization if enabled
	if (bShowPerceptionRadius || bShowSteeringForces)
	{
		DrawDebugVisualization(Separation, Alignment, Cohesion, TargetAttraction);
	}
}

void UBoidsComponent::GetNearbyBoids(TArray<ASwarmEnemy*>& OutNeighbors)
{
	OutNeighbors.Empty();

	if (!OwnerEnemy)
	{
		return;
	}

	// Setup for sphere overlap
	TArray<AActor*> OverlappingActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerEnemy);

	// Perform sphere overlap to find nearby actors
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		OwnerEnemy->GetActorLocation(),
		PerceptionRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>(), // Empty = all object types
		ASwarmEnemy::StaticClass(),
		ActorsToIgnore,
		OverlappingActors
	);

	// Filter results to only include SwarmEnemy actors
	for (AActor* Actor : OverlappingActors)
	{
		ASwarmEnemy* SwarmEnemy = Cast<ASwarmEnemy>(Actor);
		if (SwarmEnemy && SwarmEnemy != OwnerEnemy)
		{
			OutNeighbors.Add(SwarmEnemy);
		}
	}
}

FVector UBoidsComponent::CalculateSeparation(const TArray<ASwarmEnemy*>& Neighbors)
{
	if (Neighbors.Num() == 0 || !OwnerEnemy)
	{
		return FVector::ZeroVector;
	}

	FVector SeparationForce = FVector::ZeroVector;
	FVector MyLocation = OwnerEnemy->GetActorLocation();

	// For each neighbor, calculate a repulsion force
	for (ASwarmEnemy* Neighbor : Neighbors)
	{
		if (!Neighbor)
		{
			continue;
		}

		FVector ToNeighbor = MyLocation - Neighbor->GetActorLocation();
		float Distance = ToNeighbor.Size();

		if (Distance > 0.0f && Distance < PerceptionRadius)
		{
			// Inverse square law: closer neighbors have stronger repulsion
			float Force = 1.0f / (Distance * Distance);
			ToNeighbor.Normalize();
			SeparationForce += ToNeighbor * Force;
		}
	}

	// Average the force
	if (Neighbors.Num() > 0)
	{
		SeparationForce /= Neighbors.Num();
	}

	return SeparationForce;
}

FVector UBoidsComponent::CalculateAlignment(const TArray<ASwarmEnemy*>& Neighbors)
{
	if (Neighbors.Num() == 0 || !OwnerEnemy)
	{
		return FVector::ZeroVector;
	}

	FVector AverageVelocity = FVector::ZeroVector;

	// Calculate average velocity of all neighbors
	for (ASwarmEnemy* Neighbor : Neighbors)
	{
		if (!Neighbor)
		{
			continue;
		}

		AverageVelocity += Neighbor->GetVelocity();
	}

	AverageVelocity /= Neighbors.Num();

	// Steering force is the difference between average velocity and our current velocity
	FVector SteerForce = AverageVelocity - OwnerEnemy->GetVelocity();
	
	return SteerForce;
}

FVector UBoidsComponent::CalculateCohesion(const TArray<ASwarmEnemy*>& Neighbors)
{
	if (Neighbors.Num() == 0 || !OwnerEnemy)
	{
		return FVector::ZeroVector;
	}

	FVector CenterOfMass = FVector::ZeroVector;

	// Calculate center of mass of all neighbors
	for (ASwarmEnemy* Neighbor : Neighbors)
	{
		if (!Neighbor)
		{
			continue;
		}

		CenterOfMass += Neighbor->GetActorLocation();
	}

	CenterOfMass /= Neighbors.Num();

	// Steering force towards center of mass
	FVector ToCenterOfMass = CenterOfMass - OwnerEnemy->GetActorLocation();
	
	return ToCenterOfMass;
}

FVector UBoidsComponent::CalculateTargetAttraction()
{
	if (!OwnerEnemy)
	{
		return FVector::ZeroVector;
	}

	// If target location is zero, no attraction
	if (TargetLocation.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	// Calculate direction to target
	FVector ToTarget = TargetLocation - OwnerEnemy->GetActorLocation();
	
	// Normalize and return as steering force
	return ToTarget.GetSafeNormal() * MaxSpeed;
}

void UBoidsComponent::ApplySteeringForce(const FVector& SteeringForce, float DeltaTime)
{
	if (!OwnerEnemy || !MovementComponent)
	{
		return;
	}

	// Clamp steering force to maximum force
	FVector ClampedForce = SteeringForce.GetClampedToMaxSize(MaxForce);

	// Get current velocity
	FVector CurrentVelocity = OwnerEnemy->GetVelocity();

	// Calculate desired velocity
	FVector DesiredVelocity = CurrentVelocity + (ClampedForce * DeltaTime);

	// Clamp to max speed
	DesiredVelocity = DesiredVelocity.GetClampedToMaxSize(MaxSpeed);

	// Apply the velocity to the character movement component
	// Note: CharacterMovementComponent works differently than simple velocity assignment
	// We'll use AddInputVector to influence movement direction
	if (!DesiredVelocity.IsNearlyZero())
	{
		FVector MovementDirection = DesiredVelocity.GetSafeNormal();
		OwnerEnemy->AddMovementInput(MovementDirection, 1.0f);
	}
}

void UBoidsComponent::DrawDebugVisualization(const FVector& Separation, const FVector& Alignment, const FVector& Cohesion, const FVector& TargetAttraction)
{
	if (!OwnerEnemy)
	{
		return;
	}

	FVector MyLocation = OwnerEnemy->GetActorLocation();

	// Draw perception radius
	if (bShowPerceptionRadius)
	{
		DrawDebugSphere(GetWorld(), MyLocation, PerceptionRadius, 16, FColor::Yellow, false, -1.0f, 0, 2.0f);
	}

	// Draw steering forces
	if (bShowSteeringForces)
	{
		// Separation = Red
		DrawDebugLine(GetWorld(), MyLocation, MyLocation + Separation, FColor::Red, false, -1.0f, 0, 3.0f);

		// Alignment = Green
		DrawDebugLine(GetWorld(), MyLocation, MyLocation + Alignment, FColor::Green, false, -1.0f, 0, 3.0f);

		// Cohesion = Blue
		DrawDebugLine(GetWorld(), MyLocation, MyLocation + Cohesion, FColor::Blue, false, -1.0f, 0, 3.0f);

		// Target Attraction = Magenta
		DrawDebugLine(GetWorld(), MyLocation, MyLocation + TargetAttraction, FColor::Magenta, false, -1.0f, 0, 3.0f);
	}
}
