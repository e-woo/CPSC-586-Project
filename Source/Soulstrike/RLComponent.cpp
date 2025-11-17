#include "RLComponent.h"
#include "EliteEnemy.h"
#include "EliteArcher.h"
#include "EliteAssassin.h"
#include "ElitePaladin.h"
#include "EliteGiant.h"
#include "EliteHealer.h"
#include "EnemyLogicManager.h"
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

	// Default stats (will be overridden by Elite behavior object)
	AttackDamage = 10.0f;
	MaxAttackRange = 500.0f;
	AttackWindupDuration = 0.3f;
	AttackCooldown = 0.5f;

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
	OwnerCharacter = Cast<ACharacter>(InPawn);
	if (OwnerCharacter)
	{
		// Get initial health from Blueprint variable "CurrentHealth"
		FProperty* HealthProperty = OwnerCharacter->GetClass()->FindPropertyByName(TEXT("CurrentHealth"));
		if (HealthProperty)
		{
			float* HealthPtr = HealthProperty->ContainerPtrToValuePtr<float>(OwnerCharacter);
			if (HealthPtr)
			{
				PreviousHealth = *HealthPtr;
			}
		}
		else
		{
			PreviousHealth = 100.0f;
		}

		// Detect elite type by name and create corresponding C++ behavior object
		FString PawnName = InPawn->GetName();
		
		if (PawnName.Contains("Archer"))
		{
			EliteBehavior = NewObject<UEliteArcher>(this, UEliteArcher::StaticClass());
		}
		else if (PawnName.Contains("Assassin"))
		{
			EliteBehavior = NewObject<UEliteAssassin>(this, UEliteAssassin::StaticClass());
		}
		else if (PawnName.Contains("Paladin"))
		{
			EliteBehavior = NewObject<UElitePaladin>(this, UElitePaladin::StaticClass());
		}
		else if (PawnName.Contains("Giant"))
		{
			EliteBehavior = NewObject<UEliteGiant>(this, UEliteGiant::StaticClass());
		}
		else if (PawnName.Contains("Healer"))
		{
			EliteBehavior = NewObject<AEliteHealer>(this, AEliteHealer::StaticClass());
		}
		else
		{
			// Fallback to base
			EliteBehavior = NewObject<AEliteEnemy>(this, AEliteEnemy::StaticClass());
		}

		// Copy stats from behavior object
		if (EliteBehavior)
		{
			AttackDamage = EliteBehavior->AttackDamage;
			MaxAttackRange = EliteBehavior->MaxAttackRange;
			AttackWindupDuration = EliteBehavior->AttackWindupDuration;
			AttackCooldown = EliteBehavior->AttackCooldown;

			UE_LOG(LogTemp, Log, TEXT("RLComponent: Created %s behavior for %s (Damage: %.0f, Range: %.0f)"),
				*EliteBehavior->GetClass()->GetName(), *PawnName, AttackDamage, MaxAttackRange);
		}

		UE_LOG(LogTemp, Log, TEXT("RLComponent: Initialized with character %s (Initial HP: %.1f)"), 
			*OwnerCharacter->GetName(), PreviousHealth);
	}
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

	// Clean up old damage/healing history
	float CurrentTime = GetWorld()->GetTimeSeconds();
	CleanupDamageHistory(CurrentTime);

	// Update poison ticks (for Assassin)
	if (ActivePoisons.Num() > 0)
	{
		UpdatePoisons(DeltaTime);
	}

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
		
		// Log reward if significant health change
		float HealthDelta = CurrentState.SelfHealthPercentage - PreviousState.SelfHealthPercentage;
		if (FMath::Abs(HealthDelta) > 0.01f)
		{
			UE_LOG(LogTemp, Log, TEXT("RLComponent: %s health delta: %.3f, reward: %.2f"),
				*OwnerCharacter->GetName(), HealthDelta, Reward);
		}
		
		UpdateWeights(PreviousState, LastAction, Reward, CurrentState);
		LastReward = Reward;
	}

	// Select action
	EEliteAction SelectedAction = SelectAction(CurrentState);

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

	if (ClosestAllies.Num() > 0)
	{
		State.HealthOfClosestAlly = GetCharacterHealthPercentage(ClosestAllies[0]);
		State.DistanceToClosestAlly = FMath::Clamp(
			FVector::Dist(OwnerCharacter->GetActorLocation(), ClosestAllies[0]->GetActorLocation()) / MaxAllyDistance,
			0.0f, 1.0f
		);
	}

	if (ClosestAllies.Num() > 1)
	{
		State.HealthOfSecondClosestAlly = GetCharacterHealthPercentage(ClosestAllies[1]);
		State.DistanceToSecondClosestAlly = FMath::Clamp(
			FVector::Dist(OwnerCharacter->GetActorLocation(), ClosestAllies[1]->GetActorLocation()) / MaxAllyDistance,
			0.0f, 1.0f
		);
	}

	if (ClosestAllies.Num() > 2)
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
	if (!OwnerCharacter || !PlayerCharacter)
		return;

	AAIController* AIController = Cast<AAIController>(OwnerCharacter->GetController());
	if (!AIController)
		return;

	FVector OwnerLocation = OwnerCharacter->GetActorLocation();
	FVector PlayerLocation = CachedPlayerLocation;
	FVector DirectionToPlayer = (PlayerLocation - OwnerLocation).GetSafeNormal();
	FVector RightVector = FVector::CrossProduct(DirectionToPlayer, FVector::UpVector).GetSafeNormal();

	const float MoveDistance = 200.0f;

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
			PerformPrimaryAttackOnElite();
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
			PerformSecondaryAttackOnElite();
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

void URLComponent::FindClosestAllies(TArray<ACharacter*>& OutAllies, int32 NumAllies)
{
	OutAllies.Empty();

	if (!OwnerCharacter)
		return;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), FoundActors);

	TArray<TPair<float, ACharacter*>> AllyDistances;

	for (AActor* Actor : FoundActors)
	{
		ACharacter* Ally = Cast<ACharacter>(Actor);
		if (Ally && Ally != OwnerCharacter && IsCharacterAlive(Ally))
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
	if (!OwnerCharacter)
		return 0;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), FoundActors);

	int32 Count = 0;
	for (AActor* Actor : FoundActors)
	{
		ACharacter* Ally = Cast<ACharacter>(Actor);
		if (Ally && Ally != OwnerCharacter && IsCharacterAlive(Ally))
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

	FString DebugText = FString::Printf(
		TEXT("%s\nAction: %d\nReward: %.2f\nDist: %.2f\nHP: %.2f\nDPS: %.1f"),
		*StateText,
		static_cast<int32>(LastAction),
		LastReward,
		CurrentState.DistanceToPlayer,
		CurrentState.SelfHealthPercentage,
		GetAverageDPS()
	);

	// Draw with duration to persist between frames
	DrawDebugString(GetWorld(), DebugLocation, DebugText, nullptr, FColor::Yellow, 0.1f, true);
}

float URLComponent::GetCharacterHealthPercentage(ACharacter* Character) const
{
	if (!Character)
		return 0.0f;

	// Try to get CurrentHealth and MaxHealth from Blueprint
	FProperty* CurrentHealthProp = Character->GetClass()->FindPropertyByName(TEXT("CurrentHealth"));
	FProperty* MaxHealthProp = Character->GetClass()->FindPropertyByName(TEXT("MaxHealth"));

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
	
	UE_LOG(LogTemp, Log, TEXT("%s: Starting attack windup (%.2fs)..."), 
		*OwnerCharacter->GetName(), AttackWindupDuration);
}

void URLComponent::PerformSecondaryAttackOnElite()
{
	if (!EliteBehavior || !OwnerCharacter)
		return;

	// For now, only Healer has secondary attack (heal)
	if (EliteBehavior->IsA(AEliteHealer::StaticClass()))
	{
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
			// Heal the ally
			float HealAmount = 50.0f;
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

	// Special case: Assassin applies poison on hit
	if (EliteBehavior->IsA(AEliteAssassin::StaticClass()))
	{
		// Apply poison (90 damage over 3 seconds)
		FPoisonEffect NewPoison;
		NewPoison.RemainingDuration = 3.0f;
		NewPoison.TickInterval = 0.5f;
		NewPoison.DamagePerTick = 90.0f / (3.0f / 0.5f);  // 90 / 6 ticks = 15 per tick
		NewPoison.TimeSinceLastTick = 0.0f;
		
		ActivePoisons.Add(NewPoison);
		
		UE_LOG(LogTemp, Log, TEXT("Assassin: Applied poison (90 damage over 3s)"));
	}
}

void URLComponent::UpdatePoisons(float DeltaTime)
{
	if (!OwnerCharacter)
		return;

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
		return;

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0);
	AEnemyLogicManager* EnemyLogicMgr = Cast<AEnemyLogicManager>(
		UGameplayStatics::GetActorOfClass(World, AEnemyLogicManager::StaticClass())
	);

	for (int32 i = ActivePoisons.Num() - 1; i >= 0; --i)
	{
		FPoisonEffect& Poison = ActivePoisons[i];
		
		Poison.TimeSinceLastTick += DeltaTime;
		Poison.RemainingDuration -= DeltaTime;

		// Tick damage
		if (Poison.TimeSinceLastTick >= Poison.TickInterval)
		{
			// Record DPS for RL
			RecordDamageDealt(Poison.DamagePerTick);

			// Report poison damage to Enemy Logic Manager
			if (EnemyLogicMgr && Player)
			{
				EnemyLogicMgr->ReportDamageToPlayer(Player, Poison.DamagePerTick, OwnerCharacter);
				UE_LOG(LogTemp, Log, TEXT("Assassin poison tick: %.0f damage"), Poison.DamagePerTick);
			}
			
			Poison.TimeSinceLastTick = 0.0f;
		}

		// Remove expired poisons
		if (Poison.RemainingDuration <= 0.0f)
		{
			ActivePoisons.RemoveAt(i);
			UE_LOG(LogTemp, Log, TEXT("Assassin poison expired"));
		}
	}
}
