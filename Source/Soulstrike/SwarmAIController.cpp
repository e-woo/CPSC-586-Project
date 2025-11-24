#include "SwarmAIController.h"
#include <Engine.h>
#include "Util/LoadBP.h"

TSubclassOf<APawn> SwarmEnemyClass;

ASwarmAIController::ASwarmAIController()
{
	// Set this controller to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;

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
	TargetLocation = FVector::ZeroVector;
	LoadBP::LoadClass("/Game/ThirdPersonBP/Blueprints/SwarmAI/BP_SwarmEnemy.BP_SwarmEnemy", SwarmEnemyClass);
}

void ASwarmAIController::BeginPlay()
{
	Super::BeginPlay();
}

void ASwarmAIController::OnPossess(APawn* InPawn)
{
	FString Folder = InPawn->GetFolderPath().ToString();
	FString Last = FPaths::GetCleanFilename(Folder);

	UE_LOG(LogTemp, Warning, TEXT("Possessed Pawn: %s in Swarm Group: %s"), *InPawn->GetName(), *Last);
	if (!SwarmMap.Contains(Last))
	{
		UE_LOG(LogTemp, Warning, TEXT("Creating new Swarm Group: %s"), *Last);
		SwarmMap.Add(Last, TArray<TWeakObjectPtr<APawn>>());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Adding to existing Swarm Group: %s"), *Last);
	}
	SwarmMap[Last].Add(InPawn);

	Super::OnPossess(InPawn);
}

void ASwarmAIController::OnUnPossess()
{
	Super::OnUnPossess();
}


void ASwarmAIController::Tick(float DeltaTime)
{
	for (auto& Entry : SwarmMap)
	{
		// Find nearby neighbors
		TArray<TWeakObjectPtr<APawn>>& Neighbors = Entry.Value;
		for (auto& Target : Neighbors)
		{
			UE_LOG(LogTemp, Warning, TEXT("Processing Boid: %s"), *Target->GetName());
			APawn* TargetRaw = Target.Get();
			TArray<TWeakObjectPtr<APawn>> Neighbors2 = Neighbors;
			GetNearbyBoids(TargetRaw, Neighbors2);

			// Calculate individual steering forces
			FVector Separation = FVector::ZeroVector;
			FVector Alignment = FVector::ZeroVector;
			FVector Cohesion = FVector::ZeroVector;

			// Only calculate flock forces if we have neighbors
			if (Neighbors2.Num() > 0)
			{
				Separation = CalculateSeparation(TargetRaw, Neighbors2) * SeparationWeight;
				Alignment = CalculateAlignment(TargetRaw, Neighbors2) * AlignmentWeight;
				Cohesion = CalculateCohesion(TargetRaw, Neighbors2) * CohesionWeight;
			}

			// Calculate target attraction force
			FVector TargetAttraction = CalculateTargetAttraction(TargetRaw) * TargetAttractionWeight;

			// Combine all forces
			FVector CombinedForce = Separation + Alignment + Cohesion + TargetAttraction;

			// Apply the steering force
			ApplySteeringForce(TargetRaw, CombinedForce, DeltaTime);

			// Draw debug visualization if enabled
			if (bShowPerceptionRadius || bShowSteeringForces)
			{
				DrawDebugVisualization(TargetRaw, Separation, Alignment, Cohesion, TargetAttraction);
			}
		}

	}


	Super::Tick(DeltaTime);
}

void ASwarmAIController::GetNearbyBoids(APawn* Target, TArray<TWeakObjectPtr<APawn>>& OutNeighbors)
{
	OutNeighbors.Empty();

	// Setup for sphere overlap
	TArray<AActor*> OverlappingActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Target);

	// Perform sphere overlap to find nearby actors
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		Target->GetActorLocation(),
		PerceptionRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>(), // Empty = all object types
		SwarmEnemyClass,
		ActorsToIgnore,
		OverlappingActors
	);

	// Filter results to only include SwarmEnemy actors
	for (AActor* Actor : OverlappingActors)
	{
		APawn* SwarmEnemy = Cast<APawn>(Actor);
		if (SwarmEnemy && SwarmEnemy != Target)
		{
			OutNeighbors.Add(SwarmEnemy);
		}
	}
}

FVector ASwarmAIController::CalculateSeparation(APawn* Target, const TArray<TWeakObjectPtr<APawn>>& Neighbors)
{
	if (Neighbors.Num() == 0 || !Target)
	{
		return FVector::ZeroVector;
	}

	FVector SeparationForce = FVector::ZeroVector;
	FVector MyLocation = Target->GetActorLocation();

	// For each neighbor, calculate a repulsion force
	for (TWeakObjectPtr<APawn> Neighbor : Neighbors)
	{
		if (!Neighbor.IsValid())
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

FVector ASwarmAIController::CalculateAlignment(APawn* Target, const TArray<TWeakObjectPtr<APawn>>& Neighbors)
{
	if (Neighbors.Num() == 0 || !Target)
	{
		return FVector::ZeroVector;
	}

	FVector AverageVelocity = FVector::ZeroVector;

	// Calculate average velocity of all neighbors
	for (TWeakObjectPtr<APawn> Neighbor : Neighbors)
	{
		if (!Neighbor.IsValid())
		{
			continue;
		}

		AverageVelocity += Neighbor->GetVelocity();
	}

	AverageVelocity /= Neighbors.Num();

	// Steering force is the difference between average velocity and our current velocity
	FVector SteerForce = AverageVelocity - Target->GetVelocity();

	return SteerForce;
}

FVector ASwarmAIController::CalculateCohesion(APawn* Target, const TArray<TWeakObjectPtr<APawn>>& Neighbors)
{
	if (Neighbors.Num() == 0 || !Target)
	{
		return FVector::ZeroVector;
	}

	FVector CenterOfMass = FVector::ZeroVector;

	// Calculate center of mass of all neighbors
	for (TWeakObjectPtr<APawn> Neighbor : Neighbors)
	{
		if (!Neighbor.IsValid())
		{
			continue;
		}

		CenterOfMass += Neighbor->GetActorLocation();
	}

	CenterOfMass /= Neighbors.Num();

	// Steering force towards center of mass
	FVector ToCenterOfMass = CenterOfMass - Target->GetActorLocation();

	return ToCenterOfMass;
}

FVector ASwarmAIController::CalculateTargetAttraction(APawn* Target)
{
	if (!Target)
	{
		return FVector::ZeroVector;
	}

	// If target location is zero, no attraction
	if (TargetLocation.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	// Calculate direction to target
	FVector ToTarget = TargetLocation - Target->GetActorLocation();

	// Normalize and return as steering force
	return ToTarget.GetSafeNormal() * MaxSpeed;
}

void ASwarmAIController::ApplySteeringForce(APawn* Target, const FVector& SteeringForce, float DeltaTime)
{
	if (!Target || !Target->GetMovementComponent())
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplySteeringForce: Invalid Target or Movement Component"));
		return;
	}

	// Clamp steering force to maximum force
	FVector ClampedForce = SteeringForce.GetClampedToMaxSize(MaxForce);

	// Get current velocity
	FVector CurrentVelocity = Target->GetVelocity();

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
		Target->AddMovementInput(MovementDirection, 1.0f);
		UE_LOG(LogTemp, Log, TEXT("Applying Movement Input: %s"), *MovementDirection.ToString());
	}
}

void ASwarmAIController::DrawDebugVisualization(APawn* Target, const FVector& Separation, const FVector& Alignment, const FVector& Cohesion, const FVector& TargetAttraction)
{
	if (!Target)
	{
		return;
	}

	FVector MyLocation = Target->GetActorLocation();

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
