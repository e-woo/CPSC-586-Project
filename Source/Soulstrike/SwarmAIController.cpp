#include "SwarmAIController.h"
#include <Engine.h>
#include "Util/LoadBP.h"
#include "EnemyLogicManager.h"

TMap<FString, TArray<TWeakObjectPtr<ACharacter>>> ASwarmAIController::SwarmMap;

ASwarmAIController::ASwarmAIController()
{
	// Set this controller to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;
}

void ASwarmAIController::BeginPlay()
{
	FTimerDelegate SwarmTimerDelegate;
	FTimerHandle SwarmTimerHandle;

	SwarmTimerDelegate.BindUObject(this, &ASwarmAIController::MoveSwarms);
	GetWorldTimerManager().SetTimer(SwarmTimerHandle, SwarmTimerDelegate, 0.01f, true);

	FTimerDelegate SwarmAttackTimerDelegate;
	FTimerHandle SwarmAttackTimerHandle;

	SwarmAttackTimerDelegate.BindUObject(this, &ASwarmAIController::ProcessSwarmAttacks);
	GetWorldTimerManager().SetTimer(SwarmAttackTimerHandle, SwarmAttackTimerDelegate, 0.1f, true);
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
					Separation += ToSelf.GetSafeNormal();
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

void ASwarmAIController::ProcessSwarmAttacks()
{
	UWorld* World = GetWorld();
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0);

	if (!World || !Player) return;

	for (auto& Entry : SwarmMap)
	{
		for (auto& Enemy : Entry.Value)
		{
			if (!Enemy.IsValid()) continue;
			float DistToPlayer = FVector::Dist(Enemy->GetActorLocation(), Player->GetActorLocation());
			if (DistToPlayer > MaxAttackRange) continue;


			if (WindingUpMap.Contains(Enemy) && WindingUpMap[Enemy])
				continue;

			WindingUpMap.Add(Enemy, true);
			FTimerHandle& AttackTimerHandle = ActiveWindupTimerMap.FindOrAdd(Enemy);

			FTimerDelegate AttackTimerDelegate;

			AttackTimerDelegate.BindUObject(this, &ASwarmAIController::OnAttackWindupComplete, Enemy.Get(), Player);

			GetWorldTimerManager().SetTimer(AttackTimerHandle, AttackTimerDelegate, AttackWindupDuration, false);

			UFunction* AttackStartFunc = Enemy->FindFunction(FName("OnAttackStart"));
			if (AttackStartFunc)
			{
				FAttackStartParams Params{};
				Params.WindupDuration = AttackWindupDuration;

				Enemy->ProcessEvent(AttackStartFunc, &Params);
			}
		}
	}
}

void ASwarmAIController::OnAttackWindupComplete(ACharacter* Attacker, ACharacter* Target)
{
	UWorld* World = GetWorld();
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0);

	if (!World || !Player) return;

	// Clear windup flag
	WindingUpMap.FindOrAdd(Attacker) = false;

	// Clear timer
	if (ActiveWindupTimerMap.Contains(Attacker))
	{
		GetWorldTimerManager().ClearTimer(ActiveWindupTimerMap[Attacker]);
	}

	float DistToPlayer = FVector::Dist(Attacker->GetActorLocation(), Player->GetActorLocation());
	if (DistToPlayer > MaxAttackRange)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Attack missed! Player dodged out of range during windup (%.1f/%.1f)"),
			*Attacker->GetName(), DistToPlayer, MaxAttackRange);
		return;
	}

	AEnemyLogicManager* EnemyLogicMgr = Cast<AEnemyLogicManager>(UGameplayStatics::GetActorOfClass(World, AEnemyLogicManager::StaticClass()));

	if (EnemyLogicMgr)
	{
		EnemyLogicMgr->ReportDamageToPlayer(Player, AttackDamage, Attacker);
	}
}