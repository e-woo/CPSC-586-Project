#include "RLComponent.h"
#include "EliteEnemy.h"
#include "EliteArcher.h"
#include "EliteAssassin.h"
#include "ElitePaladin.h"
#include "EliteGiant.h"
#include "EliteHealer.h"
#include "EnemyLogicManager.h"
#include "SoulstrikeGameInstance.h"
#include "QLearningBrain.h"
#include "WeightManager.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"

URLComponent::URLComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Default hyperparameters
	Alpha = 0.5f;
	Gamma = 0.95f;
	Epsilon = 0.2f;
	EpsilonDecayRate = 0.0f;

	// Default stats (will be overridden by Elite behavior object)
	AttackDamage = 10.0f;
	MaxAttackRange = 500.0f;
	AttackWindupDuration = 0.3f;
	AttackCooldown = 0.5f;
	MovementSpeed = 400.0f; 

	// Debug
	bDebugMode = false;

	// Internal state
	TimeSinceLastPrimaryAttack = 0.0f;
	TimeSinceLastDamageTaken = 999.0f;
	PreviousHealth = 0.0f;

	LastAction = EEliteAction::Move_Towards_Player;
	LastReward = 0.0f;

	// Action persistence (smoother movement)
	ActionPersistenceTimer = 0.0f;
	MinActionDuration = 0.3f; // Hold each action for at least 0.3 seconds
	PendingAction = EEliteAction::Move_Towards_Player;

	// Attack state machine
	AttackState = EAttackState::Normal;
	AttackTimer = 0.0f;
	HealTarget = nullptr;

	// Delta tracking
	PreviousDPS = 0.0f;
	PreviousHPS = 0.0f;
	PreviousDistanceToPlayer = 0.0f;
	ActualDistanceToPlayer = 0.0f;

	// Stats polling (separate timer)
	StatsPollTimer = 0.0f;
	StatsPollInterval = 1.0f; // Poll every 1 second
}

void URLComponent::BeginPlay()
{
	Super::BeginPlay();

	// Get the player character
	PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	// Subscribe to player position updates from GameInstance
	USoulstrikeGameInstance* GameInstance = Cast<USoulstrikeGameInstance>(GetWorld()->GetGameInstance());
	if (GameInstance)
	{
		GameInstance->OnPlayerPositionUpdated.AddDynamic(this, &URLComponent::OnPlayerPositionUpdated);
		UE_LOG(LogTemp, Log, TEXT("RLComponent: Subscribed to player position updates."));
	}
	
	// Cache initial player location
	if (PlayerCharacter)
	{
		CachedPlayerLocation = PlayerCharacter->GetActorLocation();
	}
}

void URLComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Save weights to WeightManager when this elite dies (soul preserved)
	if (Brain.IsValid())
	{
		UWeightManager* WeightMgr = UWeightManager::Get(GetWorld());
		if (WeightMgr)
		{
			WeightMgr->SaveWeights(EliteType, Brain->GetWeights());
			UE_LOG(LogTemp, Log, TEXT("RLComponent: %s saved learned weights for future souls"), *OwnerCharacter->GetName());
		}
	}

	Super::EndPlay(EndPlayReason);
}

void URLComponent::Initialize(APawn* InPawn)
{
	OwnerCharacter = Cast<ACharacter>(InPawn);
	if (!OwnerCharacter)
		return;

	// Read initial health for damage detection
	UClass* CharacterClass = OwnerCharacter->GetClass();
	auto ReadFloatProperty = [CharacterClass](ACharacter* Character, const FName& PropertyName, float DefaultValue) -> float {
		FProperty* Property = CharacterClass->FindPropertyByName(PropertyName);
		if (Property)
		{
			float* ValuePtr = Property->ContainerPtrToValuePtr<float>(Character);
			if (ValuePtr)
				return *ValuePtr;
		}
		return DefaultValue;
	};
	PreviousHealth = ReadFloatProperty(OwnerCharacter, TEXT("CurrentHealth"), 100.0f);

	// Detect elite type by name for behavior logic (poison, healing, etc.)
	FString PawnName = InPawn->GetName();
	
	UClass* EliteClass = nullptr;
	if (PawnName.Contains("Archer"))
	{
		EliteClass = AEliteArcher::StaticClass();
		EliteType = EEliteType::Archer;
	}
	else if (PawnName.Contains("Assassin"))
	{
		EliteClass = AEliteAssassin::StaticClass();
		EliteType = EEliteType::Assassin;
	}
	else if (PawnName.Contains("Paladin"))
	{
		EliteClass = AElitePaladin::StaticClass();
		EliteType = EEliteType::Paladin;
	}
	else if (PawnName.Contains("Giant"))
	{
		EliteClass = AEliteGiant::StaticClass();
		EliteType = EEliteType::Giant;
	}
	else if (PawnName.Contains("Healer"))
	{
		EliteClass = AEliteHealer::StaticClass();
		EliteType = EEliteType::Healer;
	}
	else
	{
		EliteClass = AEliteEnemy::StaticClass();
		EliteType = EEliteType::Archer; // Default
	}

	// Store elite class for type checking (poison, healing)
	if (EliteClass)
	{
		EliteBehavior = Cast<AEliteEnemy>(EliteClass->GetDefaultObject());
	}

	// Read initial stats
	PollAndUpdateStats();

	// Initialize Q-Learning Brain
	Brain = MakeShared<FQLearningBrain>();
	
	// Try to load weights from WeightManager (learning from previous souls)
	UWeightManager* WeightMgr = UWeightManager::Get(GetWorld());
	if (WeightMgr && WeightMgr->HasWeights(EliteType))
	{
		Brain->LoadWeights(WeightMgr->LoadWeights(EliteType));
		UE_LOG(LogTemp, Log, TEXT("RLComponent: %s loaded learned weights from previous souls"), *OwnerCharacter->GetName());
	}
	else
	{
		// First spawn of this elite type - initialize with default biased weights
		Brain->InitializeWeights();
		UE_LOG(LogTemp, Log, TEXT("RLComponent: %s initialized with fresh weights (first soul)"), *OwnerCharacter->GetName());
	}

	UE_LOG(LogTemp, Log, TEXT("RLComponent: Initialized %s (HP: %.0f, Damage: %.0f, Range: %.0f, Windup: %.2fs, Cooldown: %.2fs)"), 
		*OwnerCharacter->GetName(), PreviousHealth, AttackDamage, MaxAttackRange, AttackWindupDuration, AttackCooldown);
}

void URLComponent::PollAndUpdateStats()
{
	if (!OwnerCharacter)
		return;

	// Read all stats from Blueprint using QLearningBrain helper
	FEliteStats NewStats = FQLearningBrain::ReadStatsFromBlueprint(OwnerCharacter);

	// Apply movement speed if changed
	UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement();
	if (MovementComp && FMath::Abs(MovementComp->MaxWalkSpeed - NewStats.MovementSpeed) > 1.0f)
	{
		MovementComp->MaxWalkSpeed = NewStats.MovementSpeed;
	}

	// Update cached stats
	MovementSpeed = NewStats.MovementSpeed;
	AttackDamage = NewStats.AttackDamage;
	MaxAttackRange = NewStats.MaxAttackRange;
	AttackWindupDuration = NewStats.AttackWindupDuration;
	AttackCooldown = NewStats.AttackCooldown;
}

void URLComponent::ExecuteRLStep(float DeltaTime)
{
	if (!OwnerCharacter || !IsCharacterAlive(OwnerCharacter))
		return;

	// Update cached player location
	if (PlayerCharacter)
	{
		CachedPlayerLocation = PlayerCharacter->GetActorLocation();
	}

	// === STATS POLLING (separate timer) ===
	StatsPollTimer += DeltaTime;
	if (StatsPollTimer >= StatsPollInterval)
	{
		PollAndUpdateStats();
		StatsPollTimer = 0.0f;
	}

	// Clean up old damage/healing history
	float CurrentTime = GetWorld()->GetTimeSeconds();
	CleanupDamageHistory(CurrentTime);

	// Update poison ticks (virtual function - will be overridden by Assassin)
	UpdatePoisons(DeltaTime);

	// === ATTACK STATE MACHINE UPDATE ===
	if (AttackState == EAttackState::Attacking)
	{
		AttackTimer += DeltaTime;
		if (AttackTimer >= AttackWindupDuration)
		{
			// Attack windup finished - now check if player is still in range and apply damage
			OnAttackWindupComplete();
			
			// Enter cooldown
			AttackState = EAttackState::OnCooldown;
			AttackTimer = 0.0f;
		}
		// Skip RL execution during attack windup, but still draw debug
		if (bDebugMode) DebugDraw();
		return;
	}
	else if (AttackState == EAttackState::OnCooldown)
	{
		AttackTimer += DeltaTime;
		if (AttackTimer >= AttackCooldown)
		{
			// Cooldown finished, ready to attack again
			AttackState = EAttackState::Normal;
			AttackTimer = 0.0f;
		}
	}

	// Update internal timers
	TimeSinceLastPrimaryAttack += DeltaTime;
	TimeSinceLastDamageTaken += DeltaTime;
	ActionPersistenceTimer += DeltaTime;

	// Check if we took damage this frame
	float CurrentHealth = GetCharacterHealthPercentage(OwnerCharacter) * 100.0f;
	if (CurrentHealth < PreviousHealth)
	{
		float HealthLost = PreviousHealth - CurrentHealth;
		UE_LOG(LogTemp, Warning, TEXT("RLComponent: %s took %.1f damage! (%.1f -> %.1f)"),
			*OwnerCharacter->GetName(), HealthLost, PreviousHealth, CurrentHealth);
		TimeSinceLastDamageTaken = 0.0f;
	}
	PreviousHealth = CurrentHealth;

	// Apply epsilon decay
	if (EpsilonDecayRate > 0.0f)
	{
		Epsilon = FMath::Max(0.01f, Epsilon - (EpsilonDecayRate * DeltaTime));
	}

	// Capture previous step metrics BEFORE building new state
	float PrevDistNorm = CurrentState.DistanceToPlayer;
	float PrevDPSCapture = GetAverageDPS();
	float PrevHPSCapture = GetAverageHPS();

	// Save previous state
	PreviousState = CurrentState;

	// Build current state
	CurrentState = BuildState();

	// Store delta tracking values using captured previous metrics
	PreviousDistanceToPlayer = PrevDistNorm;
	PreviousDPS = PrevDPSCapture;
	PreviousHPS = PrevHPSCapture;

	// If this is not the first step, update weights
	if (PreviousState.SelfHealthPercentage > 0.0f)
	{
		float Reward = CalculateReward();
		
		// Log reward if significant health change
		float HealthDelta = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
		if (FMath::Abs(HealthDelta) > 0.01f)
		{
			UE_LOG(LogTemp, Log, TEXT("RLComponent: %s health delta: %.3f, reward: %.2f"),
				*OwnerCharacter->GetName(), HealthDelta, Reward);
		}
		
		// Use Brain to update weights
		Brain->UpdateWeights(PreviousState, LastAction, Reward, CurrentState, Alpha, Gamma);
		LastReward = Reward;
	}

	// Use Brain to select action
	EEliteAction SelectedAction = Brain->SelectAction(CurrentState, Epsilon);

	// If attack action selected, check if can actually attack
	if (SelectedAction == EEliteAction::Primary_Attack || SelectedAction == EEliteAction::Secondary_Attack)
	{
		if (AttackState != EAttackState::Normal)
		{
			SelectedAction = EEliteAction::Move_Towards_Player;
		}
	}
	
	// Action persistence for smoother movement
	if (ActionPersistenceTimer >= MinActionDuration || SelectedAction != PendingAction)
	{
		if (SelectedAction != LastAction)
		{
			ActionPersistenceTimer = 0.0f;
		}
		
		ExecuteAction(SelectedAction, DeltaTime);
		LastAction = SelectedAction;
		PendingAction = SelectedAction;
	}
	else
	{
		ExecuteAction(LastAction, DeltaTime);
	}

	// Always draw debug when enabled
	if (bDebugMode)
	{
		DebugDraw();
	}
}

FRLState URLComponent::BuildState()
{
	FRLState State;

	if (!OwnerCharacter || !PlayerCharacter)
		return State;

	// Self stats
	State.SelfHealthPercentage = GetCharacterHealthPercentage(OwnerCharacter);
	State.TimeSinceLastAttack = FMath::Clamp(TimeSinceLastPrimaryAttack / 5.0f, 0.0f, 1.0f);
	State.bTookDamageRecently = (TimeSinceLastDamageTaken < 1.0f);

	// Distance to player
	float ActualDistance = FVector::Dist(OwnerCharacter->GetActorLocation(), CachedPlayerLocation);
	ActualDistanceToPlayer = ActualDistance; // Store actual distance for reward calculations
	State.bIsBeyondMaxRange = (ActualDistance > MaxAttackRange);
	State.DistanceToPlayer = FMath::Clamp(ActualDistance / MaxAttackRange, 0.0f, 1.0f);

	// Player stats
	State.PlayerHealthPercentage = 1.0f; // TODO: Get actual player health

	// Line of sight
	State.bHasLineOfSightToPlayer = HasLineOfSightToPlayer();

	// Allies
	TArray<ACharacter*> ClosestAllies;
	FindClosestAllies(ClosestAllies, 3);

	const float MaxAllyDistance = 2000.0f;

	if (ClosestAllies.Num() > 0 && ClosestAllies[0] && ClosestAllies[0]->IsValidLowLevel())
	{
		State.HealthOfClosestAlly = GetCharacterHealthPercentage(ClosestAllies[0]);
		State.DistanceToClosestAlly = FMath::Clamp(
			FVector::Dist(OwnerCharacter->GetActorLocation(), ClosestAllies[0]->GetActorLocation()) / MaxAllyDistance,
			0.0f, 1.0f
		);
	}

	if (ClosestAllies.Num() > 1 && ClosestAllies[1] && ClosestAllies[1]->IsValidLowLevel())
	{
		State.HealthOfSecondClosestAlly = GetCharacterHealthPercentage(ClosestAllies[1]);
		State.DistanceToSecondClosestAlly = FMath::Clamp(
			FVector::Dist(OwnerCharacter->GetActorLocation(), ClosestAllies[1]->GetActorLocation()) / MaxAllyDistance,
			0.0f, 1.0f
		);
	}

	if (ClosestAllies.Num() > 2 && ClosestAllies[2] && ClosestAllies[2]->IsValidLowLevel())
	{
		State.HealthOfThirdClosestAlly = GetCharacterHealthPercentage(ClosestAllies[2]);
		State.DistanceToThirdClosestAlly = FMath::Clamp(
			FVector::Dist(OwnerCharacter->GetActorLocation(), ClosestAllies[2]->GetActorLocation()) / MaxAllyDistance,
			0.0f, 1.0f
		);
	}

	// Team awareness
	int32 NearbyCount = CountNearbyAllies(1000.0f);
	State.NumNearbyAllies = FMath::Clamp((float)NearbyCount / 5.0f, 0.0f, 1.0f);

	return State;
}

void URLComponent::ExecuteAction(EEliteAction Action, float DeltaTime)
{
	if (!OwnerCharacter || !PlayerCharacter)
		return;

	AAIController* AIController = Cast<AAIController>(OwnerCharacter->GetController());
	if (!AIController)
		return;

	// Ensure movement speed from stats is applied
	if (UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement())
	{
		if (FMath::Abs(MoveComp->MaxWalkSpeed - MovementSpeed) > 1.0f)
		{
			MoveComp->MaxWalkSpeed = MovementSpeed;
		}
		MoveComp->bOrientRotationToMovement = true;
	}
	// Focus player to force AI to face player
	AIController->SetFocus(PlayerCharacter);

	FVector OwnerLocation = OwnerCharacter->GetActorLocation();
	FVector PlayerLocation = CachedPlayerLocation;
	FVector DirectionToPlayer = (PlayerLocation - OwnerLocation).GetSafeNormal();
	FVector RightVector = FVector::CrossProduct(DirectionToPlayer, FVector::UpVector).GetSafeNormal();

	const float MoveOffset = 600.0f;

	switch (Action)
	{
	case EEliteAction::Move_Towards_Player:
	{
		AIController->MoveToActor(PlayerCharacter, 75.0f);
		break;
	}
	case EEliteAction::Move_Away_From_Player:
	{
		FVector TargetLocation = OwnerLocation - DirectionToPlayer * MoveOffset;
		AIController->MoveToLocation(TargetLocation, 50.0f);
		break;
	}
	case EEliteAction::Strafe_Left:
	{
		FVector TargetLocation = OwnerLocation - RightVector * MoveOffset;
		AIController->MoveToLocation(TargetLocation, 50.0f);
		break;
	}
	case EEliteAction::Strafe_Right:
	{
		FVector TargetLocation = OwnerLocation + RightVector * MoveOffset;
		AIController->MoveToLocation(TargetLocation, 50.0f);
		break;
	}
	case EEliteAction::Primary_Attack:
	{
		if (AttackState == EAttackState::Normal && !CurrentState.bIsBeyondMaxRange)
		{
			PerformPrimaryAttackOnElite();
			// Only reset TimeSinceLastPrimaryAttack if attack actually started (state changed)
			if (AttackState == EAttackState::Attacking)
			{
				TimeSinceLastPrimaryAttack = 0.0f;
			}
		}
		break;
	}
	case EEliteAction::Secondary_Attack:
	{
		if (AttackState == EAttackState::Normal)
		{
			PerformSecondaryAttackOnElite();
		}
		break;
	}
	}
}

void URLComponent::RecordDamageDealt(float Damage)
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	DamageHistory.Add(TPair<float, float>(CurrentTime, Damage));
}

void URLComponent::RecordHealingDone(float HealAmount)
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	HealingHistory.Add(TPair<float, float>(CurrentTime, HealAmount));
}

float URLComponent::GetAverageDPS() const
{
	if (DamageHistory.Num() == 0)
		return 0.0f;

	float TotalDamage = 0.0f;
	for (const auto& Record : DamageHistory)
	{
		TotalDamage += Record.Value;
	}

	// Return average DPS over last 5 seconds
	return TotalDamage / 5.0f;
}

float URLComponent::GetAverageHPS() const
{
	if (HealingHistory.Num() == 0)
		return 0.0f;

	float TotalHealing = 0.0f;
	for (const auto& Record : HealingHistory)
	{
		TotalHealing += Record.Value;
	}

	// Return average HPS over last 5 seconds
	return TotalHealing / 5.0f;
}

void URLComponent::CleanupDamageHistory(float CurrentTime)
{
	const float TimeWindow = 5.0f; // Keep last 5 seconds of data (changed from 1.0)

	// Clean damage history
	DamageHistory.RemoveAll([CurrentTime, TimeWindow](const TPair<float, float>& Record) {
		return (CurrentTime - Record.Key) > TimeWindow;
	});

	// Clean healing history
	HealingHistory.RemoveAll([CurrentTime, TimeWindow](const TPair<float, float>& Record) {
		return (CurrentTime - Record.Key) > TimeWindow;
	});
}

bool URLComponent::CanAttack() const
{
	return AttackState == EAttackState::Normal;
}

bool URLComponent::JustAttacked() const
{
	return TimeSinceLastPrimaryAttack < 1.5f;
}

void URLComponent::UpdatePoisons(float DeltaTime)
{
	// Base implementation does nothing - overridden by AssassinRLComponent
}

bool URLComponent::HasActivePoisonOnPlayer() const
{
	return ActivePoisons.Num() > 0;
}

float URLComponent::CalculateReward()
{
	// Base reward is 0. Override in subclasses.
	return 0.0f;
}


void URLComponent::FindClosestAllies(TArray<ACharacter*>& OutAllies, int32 NumAllies)
{
	OutAllies.Empty();

	if (!OwnerCharacter || !OwnerCharacter->IsValidLowLevel())
		return;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), FoundActors);

	TArray<TPair<float, ACharacter*>> AllyDistances;

	for (AActor* Actor : FoundActors)
	{
		ACharacter* Ally = Cast<ACharacter>(Actor);
		
		// Add validity checks to prevent crashes from dying/invalid actors
		if (Ally && Ally->IsValidLowLevel() && !Ally->IsPendingKillOrUnreachable() && 
			Ally != OwnerCharacter && IsCharacterAlive(Ally))
		{
			// Check if this character has an AI controller (is an elite)
			AAIController* AllyAI = Cast<AAIController>(Ally->GetController());
			if (AllyAI)
			{
				float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), Ally->GetActorLocation());
				AllyDistances.Add(TPair<float, ACharacter*>(Distance, Ally));
			}
		}
	}

	// Sort by distance
	AllyDistances.Sort([](const TPair<float, ACharacter*>& A, const TPair<float, ACharacter*>& B) {
		return A.Key < B.Key;
	});

	// Take the closest N
	for (int32 i = 0; i < FMath::Min(NumAllies, AllyDistances.Num()); ++i)
	{
		OutAllies.Add(AllyDistances[i].Value);
	}
}

bool URLComponent::HasLineOfSightToPlayer()
{
	if (!OwnerCharacter || !PlayerCharacter)
		return false;

	FHitResult HitResult;
	FVector Start = OwnerCharacter->GetActorLocation();
	FVector End = CachedPlayerLocation;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);

	return !bHit || HitResult.GetActor() == PlayerCharacter;
}

int32 URLComponent::CountNearbyAllies(float Radius)
{
	if (!OwnerCharacter || !OwnerCharacter->IsValidLowLevel())
		return 0;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), FoundActors);

	int32 Count = 0;
	for (AActor* Actor : FoundActors)
	{
		ACharacter* Ally = Cast<ACharacter>(Actor);
		
		// Add validity checks
		if (Ally && Ally->IsValidLowLevel() && !Ally->IsPendingKillOrUnreachable() && 
			Ally != OwnerCharacter && IsCharacterAlive(Ally))
		{
			// Check if this character has an AI controller (is an elite)
			AAIController* AllyAI = Cast<AAIController>(Ally->GetController());
			if (AllyAI)
			{
				float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), Ally->GetActorLocation());
				if (Distance <= Radius)
				{
					Count++;
				}
			}
		}
	}

	return Count;
}

void URLComponent::OnPlayerPositionUpdated(const FVector& NewPlayerPosition)
{
	CachedPlayerLocation = NewPlayerPosition;
}

void URLComponent::DebugDraw()
{
	if (!OwnerCharacter)
		return;

	FVector OwnerLocation = OwnerCharacter->GetActorLocation();
	FVector DebugLocation = OwnerLocation + FVector(0, 0, 150);

	FString StateText = "State: ";
	switch (AttackState)
	{
	case EAttackState::Normal: StateText += "Normal"; break;
	case EAttackState::Attacking: StateText += "ATTACKING"; break;
	case EAttackState::OnCooldown: StateText += "Cooldown"; break;
	}

	// Show distance and recent reward
	FString DebugText = FString::Printf(
		TEXT("%s\nAction: %d\nR: %.2f\nDist: %.2f (%.0f/%0.f)\nHP: %.2f\nDPS: %.1f\nEps: %.2f"),
		*StateText,
		static_cast<int32>(LastAction),
		LastReward,
		CurrentState.DistanceToPlayer,
		ActualDistanceToPlayer,
		MaxAttackRange,
		CurrentState.SelfHealthPercentage,
		GetAverageDPS(),
		Epsilon
	);

	DrawDebugString(GetWorld(), DebugLocation, DebugText, nullptr, FColor::Yellow, 0.1f, true);
}

float URLComponent::GetCharacterHealthPercentage(ACharacter* Character) const
{
	if (!Character || !Character->IsValidLowLevel() || Character->IsPendingKillOrUnreachable())
		return 0.0f;

	// Try to get CurrentHealth and MaxHealth from Blueprint
	UClass* CharClass = Character->GetClass();
	if (!CharClass || !CharClass->IsValidLowLevel())
		return 0.0f;

	FProperty* CurrentHealthProp = CharClass->FindPropertyByName(TEXT("CurrentHealth"));
	FProperty* MaxHealthProp = CharClass->FindPropertyByName(TEXT("MaxHealth"));

	if (CurrentHealthProp && MaxHealthProp)
	{
		float* CurrentHealthPtr = CurrentHealthProp->ContainerPtrToValuePtr<float>(Character);
		float* MaxHealthPtr = MaxHealthProp->ContainerPtrToValuePtr<float>(Character);

		if (CurrentHealthPtr && MaxHealthPtr && *MaxHealthPtr > 0.0f)
		{
			return FMath::Clamp(*CurrentHealthPtr / *MaxHealthPtr, 0.0f, 1.0f);
		}
	}

	return 1.0f; // Default assumption
}

bool URLComponent::IsCharacterAlive(ACharacter* Character) const
{
	if (!Character || !Character->IsValidLowLevel() || Character->IsPendingKillOrUnreachable())
		return false;
		
	return GetCharacterHealthPercentage(Character) > 0.0f;
}

void URLComponent::PerformPrimaryAttackOnElite()
{
	if (!EliteBehavior || !OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("RLComponent: Cannot perform attack - EliteBehavior or OwnerCharacter is null!"));
		return;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
		return;

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0);
	if (!Player)
		return;

	float DistanceToPlayer = FVector::Dist(OwnerCharacter->GetActorLocation(), Player->GetActorLocation());
	
	if (DistanceToPlayer > MaxAttackRange)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Attack cancelled! Player out of range (%.1f/%.1f)"), 
			*OwnerCharacter->GetName(), DistanceToPlayer, MaxAttackRange);
		return;
	}

	// Start windup - cache player location for later check
	AttackWindupStartPlayerLocation = Player->GetActorLocation();
	
	// Set attack state (after range check passed)
	AttackState = EAttackState::Attacking;
	AttackTimer = 0.0f;
	
	// Trigger Blueprint custom event "OnAttackStart" with WindupDuration parameter
	UFunction* AttackStartFunc = OwnerCharacter->FindFunction(FName("OnAttackStart"));
	if (AttackStartFunc)
	{
		struct FAttackStartParams
		{
			float WindupDuration;
		};
		
		FAttackStartParams Params;
		Params.WindupDuration = AttackWindupDuration;
		
		OwnerCharacter->ProcessEvent(AttackStartFunc, &Params);
	}
	
	UE_LOG(LogTemp, Log, TEXT("%s: Starting attack windup (%.2fs)..."), 
		*OwnerCharacter->GetName(), AttackWindupDuration);
}

void URLComponent::PerformSecondaryAttackOnElite()
{
	if (!EliteBehavior || !OwnerCharacter)
		return;

	// For now, only Healer has secondary attack (heal)
	if (EliteBehavior && EliteBehavior->IsA(AEliteHealer::StaticClass()))
	{
		// Read HealAmount from Blueprint
		FProperty* HealAmountProp = OwnerCharacter->GetClass()->FindPropertyByName(TEXT("HealAmount"));
		float HealAmount = 50.0f; // Default
		if (HealAmountProp)
		{
			float* HealAmountPtr = HealAmountProp->ContainerPtrToValuePtr<float>(OwnerCharacter);
			if (HealAmountPtr)
			{
				HealAmount = *HealAmountPtr;
			}
		}

		// Find lowest HP ally and heal them
		TArray<ACharacter*> ClosestAllies;
		FindClosestAllies(ClosestAllies, 3);

		ACharacter* BestTarget = nullptr;
		float LowestHP = 1.0f;

		for (ACharacter* Ally : ClosestAllies)
		{
			if (Ally && IsCharacterAlive(Ally))
			{
				float AllyHP = GetCharacterHealthPercentage(Ally);
				if (AllyHP < 0.9f && AllyHP < LowestHP)
				{
					float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), Ally->GetActorLocation());
					if (Distance <= MaxAttackRange)
					{
						LowestHP = AllyHP;
						BestTarget = Ally;
					}
				}
			}
		}

		if (BestTarget)
		{
			// Set attack state (before triggering animation)
			AttackState = EAttackState::Attacking;
			AttackTimer = 0.0f;
			
			// Trigger Blueprint custom event "OnSecondaryStart" with WindupDuration parameter
			UFunction* SecondaryStartFunc = OwnerCharacter->FindFunction(FName("OnSecondaryStart"));
			if (SecondaryStartFunc)
			{
				struct FSecondaryStartParams
				{
					float WindupDuration;
				};
				
				FSecondaryStartParams Params;
				Params.WindupDuration = AttackWindupDuration;
				
				OwnerCharacter->ProcessEvent(SecondaryStartFunc, &Params);
			}

			// Heal the ally
			FProperty* CurrentHealthProp = BestTarget->GetClass()->FindPropertyByName(TEXT("CurrentHealth"));
			FProperty* MaxHealthProp = BestTarget->GetClass()->FindPropertyByName(TEXT("MaxHealth"));
			if (CurrentHealthProp && MaxHealthProp)
			{
				float* CurrentHealthPtr = CurrentHealthProp->ContainerPtrToValuePtr<float>(BestTarget);
				float* MaxHealthPtr = MaxHealthProp->ContainerPtrToValuePtr<float>(BestTarget);
				if (CurrentHealthPtr && MaxHealthPtr)
				{
					*CurrentHealthPtr = FMath::Min(*MaxHealthPtr, *CurrentHealthPtr + HealAmount);
				}
			}
			
			RecordHealingDone(HealAmount);
			UE_LOG(LogTemp, Log, TEXT("Healer: Healed %s for %.0f HP"), *BestTarget->GetName(), HealAmount);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("%s: Secondary attack (not implemented for this elite type)"), 
			*OwnerCharacter->GetName());
	}
}

void URLComponent::OnAttackWindupComplete()
{
	if (!EliteBehavior || !OwnerCharacter)
		return;

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
		return;

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0);
	if (!Player)
		return;

	// Check if player is STILL in range after windup
	float CurrentDistanceToPlayer = FVector::Dist(OwnerCharacter->GetActorLocation(), Player->GetActorLocation());
	
	if (CurrentDistanceToPlayer > MaxAttackRange)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Attack missed! Player dodged out of range during windup (%.1f/%.1f)"), 
			*OwnerCharacter->GetName(), CurrentDistanceToPlayer, MaxAttackRange);
		return;
	}

	// Player is still in range - apply damage!
	UE_LOG(LogTemp, Log, TEXT("%s: Attack hit! (%.0f damage)"), *OwnerCharacter->GetName(), AttackDamage);

	// Record damage
	RecordDamageDealt(AttackDamage);

	// Report damage to Enemy Logic Manager
	AEnemyLogicManager* EnemyLogicMgr = Cast<AEnemyLogicManager>(
		UGameplayStatics::GetActorOfClass(World, AEnemyLogicManager::StaticClass())
	);

	if (EnemyLogicMgr && Player)
	{
		EnemyLogicMgr->ReportDamageToPlayer(Player, AttackDamage, OwnerCharacter);
	}
}
