#include "SwarmAIController.h"
#include <Engine.h>
#include "Util/LoadBP.h"
#include "EnemyLogicManager.h"

TMap<FGuid, TArray<TWeakObjectPtr<ACharacter>>> ASwarmAIController::SwarmMap;
TMap<TWeakObjectPtr<ACharacter>, bool> ASwarmAIController::WindingUpMap;
TMap<TWeakObjectPtr<ACharacter>, FTimerHandle> ASwarmAIController::ActiveWindupTimerMap;

ASwarmAIController::ASwarmAIController()
{
	// Set this controller to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;
}

void ASwarmAIController::BeginPlay()
{
	LoadBP::LoadClass("/Game/ThirdPersonBP/Blueprints/SwarmAI/BP_SwarmEnemy.BP_SwarmEnemy_C", EnemyActorClass);

	FTimerDelegate SwarmAttackTimerDelegate;
	FTimerHandle SwarmAttackTimerHandle;

	SwarmAttackTimerDelegate.BindUObject(this, &ASwarmAIController::ProcessAttack);
	GetWorldTimerManager().SetTimer(SwarmAttackTimerHandle, SwarmAttackTimerDelegate, 0.1f, true);
	Super::BeginPlay();
}

void ASwarmAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}
void ASwarmAIController::RegisterSwarmEnemy(APawn* InPawn, const FGuid& InSwarmId)
{
	if (!InPawn) return;
	this->SwarmId = InSwarmId;
	//UE_LOG(LogTemp, Warning, TEXT("Swarm Group: %s"), *InSwarmId.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("Possessed Pawn: %s in Swarm Group: %s"), *InPawn->GetName(), *Last);
	if (!SwarmMap.Contains(InSwarmId))
	{
		//UE_LOG(LogTemp, Warning, TEXT("Creating new Swarm Group: %s"), *InSwarmId.ToString());
		SwarmMap.Add(InSwarmId, TArray<TWeakObjectPtr<ACharacter>>());
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("Adding to existing Swarm Group: %s"), *InSwarmId.ToString());
	}

	ACharacter* PawnChar = Cast<ACharacter>(InPawn);
	if (PawnChar)
	{
		SwarmMap[InSwarmId].Add(PawnChar);
	}
}

void ASwarmAIController::OnUnPossess()
{
	Super::OnUnPossess();
}


void ASwarmAIController::Tick(float DeltaTime)
{
	ProcessMovement(DeltaTime);
	Super::Tick(DeltaTime);
}

void ASwarmAIController::ProcessMovement(float DeltaTime)
{
	UWorld* World = GetWorld();
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0);
	if (!Player) return;


	if (!SwarmMap.Contains(SwarmId))
	{
		return;
	}
	// Each entry represents a swarm group
	TArray<TWeakObjectPtr<ACharacter>>& Neighbors = SwarmMap[SwarmId];

	if (Neighbors.Num() == 0)
		return;

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
		return;

	AvgLocation /= ValidCount;
	AvgForward = AvgForward.GetSafeNormal();

	TWeakObjectPtr<ACharacter> Target = Cast<ACharacter>(GetPawn());
	if (!Target.IsValid()) return;
	if (WindingUpMap.Contains(Target) && WindingUpMap[Target])
		return;


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
	Target->AddMovementInput(DesiredDir, DeltaTime * 600.f);
	
	//DrawDebugLine(World, Target->GetActorLocation(), Target->GetActorLocation() + DesiredDir * 1000.f, FColor::Red, false, 0.1f, 0, 2.f);


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

void ASwarmAIController::ProcessAttack()
{
	TWeakObjectPtr<ACharacter> Enemy = Cast<ACharacter>(GetPawn());
	if (!Enemy.IsValid()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0);
	if (!Player) return;

	float DistToPlayer = FVector::Dist(Enemy->GetActorLocation(), Player->GetActorLocation());
	if (DistToPlayer > MaxAttackRange) return;

	if (WindingUpMap.Contains(Enemy) && WindingUpMap[Enemy]) return;

	WindingUpMap.Add(Enemy, true);

	FTimerHandle& AttackTimerHandle = ActiveWindupTimerMap.FindOrAdd(Enemy);
	FTimerDelegate AttackTimerDelegate;

	AttackTimerDelegate.BindUObject(this, &ASwarmAIController::OnAttackWindupComplete, Player);

	GetWorldTimerManager().SetTimer(AttackTimerHandle, AttackTimerDelegate, AttackWindupDuration, false);

	if (UFunction* AttackStartFunc = Enemy->FindFunction(FName("OnAttackStart")))
	{
		FAttackStartParams Params{};
		Params.WindupDuration = AttackWindupDuration;

		Enemy->ProcessEvent(AttackStartFunc, &Params);
	}
}

void ASwarmAIController::OnAttackWindupComplete(ACharacter* Target)
{
	if (!Target) return;

	TWeakObjectPtr<ACharacter> Enemy = Cast<ACharacter>(GetPawn());
	if (!Enemy.IsValid()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// Clear windup flag
	WindingUpMap.FindOrAdd(Enemy) = false;

	// Clear timer
	if (ActiveWindupTimerMap.Contains(Enemy))
	{
		GetWorldTimerManager().ClearTimer(ActiveWindupTimerMap[Enemy]);
	}

	float DistToPlayer = FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation());
	if (DistToPlayer > MaxAttackRange)
	{
#if UE_EDITOR
		UE_LOG(LogTemp, Warning, TEXT("%s: Attack missed! Player dodged out of range during windup (%.1f/%.1f)"),
			*Enemy->GetName(), DistToPlayer, MaxAttackRange);
#endif
		return;
	}
	AEnemyLogicManager* EnemyLogicMgr = Cast<AEnemyLogicManager>(UGameplayStatics::GetActorOfClass(World, AEnemyLogicManager::StaticClass()));

	if (EnemyLogicMgr)
	{
		EnemyLogicMgr->ReportDamageToPlayer(Target, AttackDamage, Enemy.Get());
	}
}