#include "RLComponent.h"
#include "EliteEnemy.h"
#include "SoulstrikeGameInstance.h"
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
	PreviousDistanceToPlayer = 0.0f;
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

	// Initialize weights
	InitializeWeights();
}

void URLComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void URLComponent::Initialize(APawn* InPawn)
{
	OwnerElite = Cast<AEliteEnemy>(InPawn);
	if (OwnerElite)
	{
		PreviousHealth = OwnerElite->Health;
	}
}

void URLComponent::ExecuteRLStep(float DeltaTime)
{
	if (!OwnerElite || !OwnerElite->IsAlive())
		return;

	// Update cached player location
	if (PlayerCharacter)
	{
		CachedPlayerLocation = PlayerCharacter->GetActorLocation();
	}

	// Clean up old damage/healing history
	float CurrentTime = GetWorld()->GetTimeSeconds();
	CleanupDamageHistory(CurrentTime);

	// === ATTACK STATE MACHINE UPDATE ===
	if (AttackState == EAttackState::Attacking)
	{
		AttackTimer += DeltaTime;
		if (AttackTimer >= OwnerElite->AttackDuration)
		{
			// Attack animation finished, enter cooldown
			AttackState = EAttackState::OnCooldown;
			AttackTimer = 0.0f;

			// Apply attack effects now (damage/heal)
			OwnerElite->OnAttackComplete();
		}
		// Skip RL execution during attack
		if (bDebugMode) DebugDraw();
		return;
	}
	else if (AttackState == EAttackState::OnCooldown)
	{
		AttackTimer += DeltaTime;
		if (AttackTimer >= OwnerElite->AttackCooldown)
		{
			// Cooldown finished, ready to attack again
			AttackState = EAttackState::Normal;
			AttackTimer = 0.0f;
		}
		// Continue with RL during cooldown (can move, just can't attack)
	}

	// Update internal timers
	TimeSinceLastPrimaryAttack += DeltaTime;
	TimeSinceLastDamageTaken += DeltaTime;
	ActionPersistenceTimer += DeltaTime;

	// Check if we took damage this frame
	if (OwnerElite->Health < PreviousHealth)
	{
		TimeSinceLastDamageTaken = 0.0f;
	}
	PreviousHealth = OwnerElite->Health;

	// Apply epsilon decay
	if (EpsilonDecayRate > 0.0f)
	{
		Epsilon = FMath::Max(0.01f, Epsilon - (EpsilonDecayRate * DeltaTime));
	}

	// Save previous state
	PreviousState = CurrentState;

	// Build current state
	CurrentState = BuildState();

	// Store delta tracking values
	PreviousDistanceToPlayer = CurrentState.DistanceToPlayer;
	PreviousDPS = GetAverageDPS();

	// If this is not the first step, update weights
	if (PreviousState.SelfHealthPercentage > 0.0f)
	{
		float Reward = CalculateReward();
		UpdateWeights(PreviousState, LastAction, Reward, CurrentState);
		LastReward = Reward;
	}

	// Select action (with persistence for smoother movement)
	EEliteAction SelectedAction = SelectAction(CurrentState);

	// If attack action selected, check if can actually attack
	if (SelectedAction == EEliteAction::Primary_Attack || SelectedAction == EEliteAction::Secondary_Attack)
	{
		if (AttackState != EAttackState::Normal)
		{
			// Can't attack, fallback to moving toward player
			SelectedAction = EEliteAction::Move_Towards_Player;
		}
	}
	
	// Only change action if enough time has passed (prevents jerky movement)
	if (ActionPersistenceTimer >= MinActionDuration || SelectedAction != PendingAction)
	{
		// Reset persistence timer when changing actions
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
		// Continue executing the previous action
		ExecuteAction(LastAction, DeltaTime);
	}

	// Always draw debug (even during attack/cooldown)
	if (bDebugMode)
	{
		DebugDraw();
	}
}

FRLState URLComponent::BuildState()
{
	FRLState State;

	if (!OwnerElite || !PlayerCharacter)
		return State;

	// Self stats
	State.SelfHealthPercentage = OwnerElite->GetHealthPercentage();
	State.TimeSinceLastAttack = FMath::Clamp(TimeSinceLastPrimaryAttack / 5.0f, 0.0f, 1.0f);
	State.bTookDamageRecently = (TimeSinceLastDamageTaken < 1.0f);

	// Distance to player
	float ActualDistance = FVector::Dist(OwnerElite->GetActorLocation(), CachedPlayerLocation);
	float MaxRange = OwnerElite->MaxAttackRange;
	State.bIsBeyondMaxRange = (ActualDistance > MaxRange);
	State.DistanceToPlayer = FMath::Clamp(ActualDistance / MaxRange, 0.0f, 1.0f);

	// Player stats
	ACharacter* Player = Cast<ACharacter>(PlayerCharacter);
	if (Player)
	{
		// For now, assume player has 100 health (we can make this more robust later)
		State.PlayerHealthPercentage = 1.0f; // TODO: Get actual player health
	}

	// Line of sight
	State.bHasLineOfSightToPlayer = HasLineOfSightToPlayer();

	// Allies
	TArray<AEliteEnemy*> ClosestAllies;
	FindClosestAllies(ClosestAllies, 3);

	const float MaxAllyDistance = 2000.0f;

	if (ClosestAllies.Num() > 0)
	{
		State.HealthOfClosestAlly = ClosestAllies[0]->GetHealthPercentage();
		State.DistanceToClosestAlly = FMath::Clamp(
			FVector::Dist(OwnerElite->GetActorLocation(), ClosestAllies[0]->GetActorLocation()) / MaxAllyDistance,
			0.0f, 1.0f
		);
	}

	if (ClosestAllies.Num() > 1)
	{
		State.HealthOfSecondClosestAlly = ClosestAllies[1]->GetHealthPercentage();
		State.DistanceToSecondClosestAlly = FMath::Clamp(
			FVector::Dist(OwnerElite->GetActorLocation(), ClosestAllies[1]->GetActorLocation()) / MaxAllyDistance,
			0.0f, 1.0f
		);
	}

	if (ClosestAllies.Num() > 2)
	{
		State.HealthOfThirdClosestAlly = ClosestAllies[2]->GetHealthPercentage();
		State.DistanceToThirdClosestAlly = FMath::Clamp(
			FVector::Dist(OwnerElite->GetActorLocation(), ClosestAllies[2]->GetActorLocation()) / MaxAllyDistance,
			0.0f, 1.0f
		);
	}

	// Team awareness
	int32 NearbyCount = CountNearbyAllies(1000.0f);
	State.NumNearbyAllies = FMath::Clamp((float)NearbyCount / 5.0f, 0.0f, 1.0f);

	return State;
}

float URLComponent::CalculateQValue(const FRLState& State, EEliteAction Action)
{
	if (!Weights.Contains(Action))
		return 0.0f;

	TMap<FName, float> Features = ExtractFeatures(State);
	TMap<FName, float>& ActionWeights = Weights[Action];

	float QValue = 0.0f;
	for (const auto& FeaturePair : Features)
	{
		FName FeatureName = FeaturePair.Key;
		float FeatureValue = FeaturePair.Value;

		if (ActionWeights.Contains(FeatureName))
		{
			QValue += ActionWeights[FeatureName] * FeatureValue;
		}
	}

	return QValue;
}

EEliteAction URLComponent::SelectAction(const FRLState& State)
{
	// Epsilon-greedy policy
	float RandomValue = FMath::FRand();
	if (RandomValue < Epsilon)
	{
		// Explore: choose random action
		int32 RandomIndex = FMath::RandRange(0, 5); // 6 actions
		return static_cast<EEliteAction>(RandomIndex);
	}
	else
	{
		// Exploit: choose action with highest Q-value
		EEliteAction BestAction = EEliteAction::Move_Towards_Player;
		float BestQValue = -FLT_MAX;

		for (int32 i = 0; i < 6; ++i)
		{
			EEliteAction Action = static_cast<EEliteAction>(i);
			float QValue = CalculateQValue(State, Action);
			if (QValue > BestQValue)
			{
				BestQValue = QValue;
				BestAction = Action;
			}
		}

		return BestAction;
	}
}

void URLComponent::ExecuteAction(EEliteAction Action, float DeltaTime)
{
	if (!OwnerElite || !PlayerCharacter)
		return;

	AAIController* AIController = Cast<AAIController>(OwnerElite->GetController());
	if (!AIController)
		return;

	FVector OwnerLocation = OwnerElite->GetActorLocation();
	FVector PlayerLocation = CachedPlayerLocation;
	FVector DirectionToPlayer = (PlayerLocation - OwnerLocation).GetSafeNormal();
	FVector RightVector = FVector::CrossProduct(DirectionToPlayer, FVector::UpVector).GetSafeNormal();

	const float MoveDistance = 200.0f; // How far to move per action

	switch (Action)
	{
	case EEliteAction::Move_Towards_Player:
	{
		FVector TargetLocation = OwnerLocation + DirectionToPlayer * MoveDistance;
		AIController->MoveToLocation(TargetLocation, 50.0f);
		break;
	}
	case EEliteAction::Move_Away_From_Player:
	{
		FVector TargetLocation = OwnerLocation - DirectionToPlayer * MoveDistance;
		AIController->MoveToLocation(TargetLocation, 50.0f);
		break;
	}
	case EEliteAction::Strafe_Left:
	{
		FVector TargetLocation = OwnerLocation - RightVector * MoveDistance;
		AIController->MoveToLocation(TargetLocation, 50.0f);
		break;
	}
	case EEliteAction::Strafe_Right:
	{
		FVector TargetLocation = OwnerLocation + RightVector * MoveDistance;
		AIController->MoveToLocation(TargetLocation, 50.0f);
		break;
	}
	case EEliteAction::Primary_Attack:
	{
		if (AttackState == EAttackState::Normal && !CurrentState.bIsBeyondMaxRange)
		{
			// Start attack
			OwnerElite->PerformPrimaryAttack();
			AttackState = EAttackState::Attacking;
			AttackTimer = 0.0f;
			TimeSinceLastPrimaryAttack = 0.0f;
		}
		break;
	}
	case EEliteAction::Secondary_Attack:
	{
		if (AttackState == EAttackState::Normal && !CurrentState.bIsBeyondMaxRange)
		{
			// Start secondary attack (heal for healer, or other abilities)
			OwnerElite->PerformSecondaryAttack();
			AttackState = EAttackState::Attacking;
			AttackTimer = 0.0f;
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

	// Return damage over last second
	return TotalDamage; // Already cleaned to only include last 1 second
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

	// Return healing over last second
	return TotalHealing; // Already cleaned to only include last 1 second
}

void URLComponent::CleanupDamageHistory(float CurrentTime)
{
	const float TimeWindow = 1.0f; // Keep last 1 second of data

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

float URLComponent::CalculateReward()
{
	// Base reward is 0. Override in subclasses.
	return 0.0f;
}

void URLComponent::UpdateWeights(const FRLState& OldState, EEliteAction Action, float Reward, const FRLState& NewState)
{
	if (!Weights.Contains(Action))
		return;

	// Calculate current Q-value for old state-action
	float OldQValue = CalculateQValue(OldState, Action);

	// Find max Q-value for new state
	float MaxNewQValue = -FLT_MAX;
	for (int32 i = 0; i < 6; ++i)
	{
		EEliteAction NextAction = static_cast<EEliteAction>(i);
		float QValue = CalculateQValue(NewState, NextAction);
		MaxNewQValue = FMath::Max(MaxNewQValue, QValue);
	}

	// TD Error: reward + gamma * max(Q(s',a')) - Q(s,a)
	float TDError = Reward + (Gamma * MaxNewQValue) - OldQValue;

	// Update weights: w = w + alpha * TDError * feature_value
	TMap<FName, float> Features = ExtractFeatures(OldState);
	TMap<FName, float>& ActionWeights = Weights[Action];

	for (const auto& FeaturePair : Features)
	{
		FName FeatureName = FeaturePair.Key;
		float FeatureValue = FeaturePair.Value;

		if (ActionWeights.Contains(FeatureName))
		{
			ActionWeights[FeatureName] += Alpha * TDError * FeatureValue;
		}
	}
}

void URLComponent::InitializeWeights()
{
	// Initialize weights for all actions
	TArray<EEliteAction> Actions = {
		EEliteAction::Move_Towards_Player,
		EEliteAction::Move_Away_From_Player,
		EEliteAction::Strafe_Left,
		EEliteAction::Strafe_Right,
		EEliteAction::Primary_Attack,
		EEliteAction::Secondary_Attack
	};

	// Feature names
	TArray<FName> FeatureNames = {
		"DistanceToPlayer",
		"SelfHealthPercentage",
		"TimeSinceLastAttack",
		"bIsBeyondMaxRange",
		"bTookDamageRecently",
		"PlayerHealthPercentage",
		"bHasLineOfSightToPlayer",
		"HealthOfClosestAlly",
		"DistanceToClosestAlly",
		"HealthOfSecondClosestAlly",
		"DistanceToSecondClosestAlly",
		"HealthOfThirdClosestAlly",
		"DistanceToThirdClosestAlly",
		"NumNearbyAllies"
	};

	for (EEliteAction Action : Actions)
	{
		TMap<FName, float> ActionWeights;
		for (FName FeatureName : FeatureNames)
		{
			// Initialize weights to small random values
			ActionWeights.Add(FeatureName, FMath::FRandRange(-0.1f, 0.1f));
		}
		Weights.Add(Action, ActionWeights);
	}

	// === BIASED INITIALIZATION FOR ENGAGEMENT ===
	// All elites should move toward player when out of range
	Weights[EEliteAction::Move_Towards_Player]["bIsBeyondMaxRange"] = +1.0f;  // Strong positive
	Weights[EEliteAction::Move_Towards_Player]["DistanceToPlayer"] = +0.5f;   // Mild bias toward closing gap

	// Discourage moving away when out of range
	Weights[EEliteAction::Move_Away_From_Player]["bIsBeyondMaxRange"] = -1.0f;  // Strong negative

	// Strafe is neutral/slightly negative when out of range
	Weights[EEliteAction::Strafe_Left]["bIsBeyondMaxRange"] = -0.5f;
	Weights[EEliteAction::Strafe_Right]["bIsBeyondMaxRange"] = -0.5f;

	// Bias toward attacking when cooldown ready and in range
	Weights[EEliteAction::Primary_Attack]["TimeSinceLastAttack"] = +0.8f;  // Attack when ready
	Weights[EEliteAction::Primary_Attack]["DistanceToPlayer"] = -0.5f;  // Prefer when close (low distance value)
	Weights[EEliteAction::Primary_Attack]["bIsBeyondMaxRange"] = -0.8f;  // Don't attack when out of range
}
	
TMap<FName, float> URLComponent::ExtractFeatures(const FRLState& State)
{
	TMap<FName, float> Features;

	Features.Add("DistanceToPlayer", State.DistanceToPlayer);
	Features.Add("SelfHealthPercentage", State.SelfHealthPercentage);
	Features.Add("TimeSinceLastAttack", State.TimeSinceLastAttack);
	Features.Add("bIsBeyondMaxRange", State.bIsBeyondMaxRange ? 1.0f : 0.0f);
	Features.Add("bTookDamageRecently", State.bTookDamageRecently ? 1.0f : 0.0f);
	Features.Add("PlayerHealthPercentage", State.PlayerHealthPercentage);
	Features.Add("bHasLineOfSightToPlayer", State.bHasLineOfSightToPlayer ? 1.0f : 0.0f);
	Features.Add("HealthOfClosestAlly", State.HealthOfClosestAlly);
	Features.Add("DistanceToClosestAlly", State.DistanceToClosestAlly);
	Features.Add("HealthOfSecondClosestAlly", State.HealthOfSecondClosestAlly);
	Features.Add("DistanceToSecondClosestAlly", State.DistanceToSecondClosestAlly);
	Features.Add("HealthOfThirdClosestAlly", State.HealthOfThirdClosestAlly);
	Features.Add("DistanceToThirdClosestAlly", State.DistanceToThirdClosestAlly);
	Features.Add("NumNearbyAllies", State.NumNearbyAllies);

	return Features;
}

void URLComponent::FindClosestAllies(TArray<AEliteEnemy*>& OutAllies, int32 NumAllies)
{
	OutAllies.Empty();

	if (!OwnerElite)
		return;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEliteEnemy::StaticClass(), FoundActors);

	TArray<TPair<float, AEliteEnemy*>> AllyDistances;

	for (AActor* Actor : FoundActors)
	{
		AEliteEnemy* Ally = Cast<AEliteEnemy>(Actor);
		if (Ally && Ally != OwnerElite && Ally->IsAlive())
		{
			float Distance = FVector::Dist(OwnerElite->GetActorLocation(), Ally->GetActorLocation());
			AllyDistances.Add(TPair<float, AEliteEnemy*>(Distance, Ally));
		}
	}

	// Sort by distance
	AllyDistances.Sort([](const TPair<float, AEliteEnemy*>& A, const TPair<float, AEliteEnemy*>& B) {
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
	if (!OwnerElite || !PlayerCharacter)
		return false;

	FHitResult HitResult;
	FVector Start = OwnerElite->GetActorLocation();
	FVector End = CachedPlayerLocation;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerElite);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);

	// If we hit the player or nothing, we have line of sight
	return !bHit || HitResult.GetActor() == PlayerCharacter;
}

int32 URLComponent::CountNearbyAllies(float Radius)
{
	if (!OwnerElite)
		return 0;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEliteEnemy::StaticClass(), FoundActors);

	int32 Count = 0;
	for (AActor* Actor : FoundActors)
	{
		AEliteEnemy* Ally = Cast<AEliteEnemy>(Actor);
		if (Ally && Ally != OwnerElite && Ally->IsAlive())
		{
			float Distance = FVector::Dist(OwnerElite->GetActorLocation(), Ally->GetActorLocation());
			if (Distance <= Radius)
			{
				Count++;
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
	if (!OwnerElite)
		return;

	FVector OwnerLocation = OwnerElite->GetActorLocation();
	FVector DebugLocation = OwnerLocation + FVector(0, 0, 150);

	// Show attack state
	FString StateText = "State: ";
	switch (AttackState)
	{
	case EAttackState::Normal: StateText += "Normal"; break;
	case EAttackState::Attacking: StateText += "ATTACKING"; break;
	case EAttackState::OnCooldown: StateText += "Cooldown"; break;
	}

	// Display current state, action, and reward
	FString DebugText = FString::Printf(
		TEXT("%s\nAction: %d\nReward: %.2f\nDist: %.2f\nHP: %.2f\nDPS: %.1f"),
		*StateText,
		static_cast<int32>(LastAction),
		LastReward,
		CurrentState.DistanceToPlayer,
		CurrentState.SelfHealthPercentage,
		GetAverageDPS()
	);

	DrawDebugString(GetWorld(), DebugLocation, DebugText, nullptr, FColor::Yellow, 0.0f, true);
}
