#include "SwarmAIController.h"
#include <Engine.h>
#include "Util/LoadBP.h"

TSubclassOf<APawn> SwarmEnemyClass;
TMap<FString, TArray<TWeakObjectPtr<ACharacter>>> ASwarmAIController::SwarmMap;

ASwarmAIController::ASwarmAIController()
{
	// Set this controller to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;

	LoadBP::LoadClass("/Game/ThirdPersonBP/Blueprints/SwarmAI/BP_SwarmEnemy.BP_SwarmEnemy", SwarmEnemyClass);
}

void ASwarmAIController::BeginPlay()
{
	FTimerDelegate SwarmTimerDelegate;
	FTimerHandle SwarmTimerHandle;

	SwarmTimerDelegate.BindUObject(this, &ASwarmAIController::MoveSwarms);
	GetWorldTimerManager().SetTimer(SwarmTimerHandle, SwarmTimerDelegate, 0.01f, true);
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
		SwarmMap.Add(Last, TArray<TWeakObjectPtr<ACharacter>>());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Adding to existing Swarm Group: %s"), *Last);
	}
	
	ACharacter* PawnChar = Cast<ACharacter>(InPawn);
	if (PawnChar)
	{
		SwarmMap[Last].Add(PawnChar);
	}

	Super::OnPossess(InPawn);
}

void ASwarmAIController::OnUnPossess()
{
	Super::OnUnPossess();
}


void ASwarmAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASwarmAIController::MoveSwarms()
{
	UWorld* World = GetWorld();
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0);
	if (!Player) return;


	for (auto& Entry : SwarmMap)
	{
		// Each entry represents a swarm group
		TArray<TWeakObjectPtr<ACharacter>>& Neighbors = Entry.Value;

		if (Neighbors.Num() == 0)
			continue;

		// Compute group statistics ---
		FVector AvgLocation = FVector::ZeroVector;
		FVector AvgForward = FVector::ZeroVector;

		int32 ValidCount = 0;
		for (auto& Target : Neighbors)
		{
			if (!Target.IsValid()) continue;
			ValidCount++;

			AvgLocation += Target->GetActorLocation();
			AvgForward += Target->GetActorForwardVector();
		}

		if (ValidCount == 0)
			continue;

		AvgLocation /= ValidCount;
		AvgForward = AvgForward.GetSafeNormal();

		for (auto& Target : Neighbors)
		{
			if (!Target.IsValid()) continue;

			FVector TargetLoc = Target->GetActorLocation();

			// Cohesion: move toward group's center
			FVector Cohesion = (AvgLocation - TargetLoc).GetSafeNormal();

			// Alignment: follow group's average forward
			FVector Alignment = AvgForward;

			// Separation: avoid getting too close to group members
			FVector Separation = FVector::ZeroVector;
			for (auto& Neighbor : Neighbors)
			{
				if (!Neighbor.IsValid() || Neighbor.Get() == Target.Get()) continue;

				FVector ToSelf = TargetLoc - Neighbor->GetActorLocation();
				float Distance = ToSelf.Size();

				if (Distance < SeparationDistance && Distance > 1.f)
				{
					// Linear falloff (stronger when closer)
					float Strength = 1.f - (Distance / SeparationDistance);
					Separation += ToSelf.GetSafeNormal() * 1;
				}
			}

			// Target: Move entire swarm toward player if within engage distance
			FVector ToPlayer = FVector::ZeroVector;
			float DistanceToPlayer = FVector::Dist(AvgLocation, Player->GetActorLocation());
			if (DistanceToPlayer <= TargetEngageDistance)
			{
				ToPlayer = (Player->GetActorLocation() - TargetLoc).GetSafeNormal();
			}

			// Compute Direction
			FVector DesiredDir =
				(Cohesion * CohesionWeight
					+ Alignment * AlignmentWeight
					+ Separation * SeparationWeight
					+ ToPlayer * TargetWeight)
				.GetSafeNormal();

			// Apply movement
			Target->AddMovementInput(DesiredDir, 1.0f);

			// Jump over obstacles
			FVector Start = Target->GetActorLocation();
			FVector End = Start + DesiredDir * 200.f;

			FCollisionQueryParams Params;
			Params.AddIgnoredActor(Target.Get());

			FHitResult Hit;
			bool bBlocked = World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);

			if (bBlocked && Target->GetMovementComponent()->IsMovingOnGround())
			{
				Target->Jump();
			}
		}
	}
}