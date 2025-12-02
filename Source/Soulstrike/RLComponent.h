#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLComponent.generated.h"

// Forward declarations
class AEliteEnemy;
class ACharacter;
class FQLearningBrain;
enum class EEliteType : uint8;

/** Poison damage-over-time effect (for Assassin) */
USTRUCT()
struct FPoisonEffect
{
	GENERATED_BODY()

	float RemainingDuration;
	float DamagePerTick;
	float TickInterval;
	float TimeSinceLastTick;

	FPoisonEffect()
		: RemainingDuration(0.0f)
		, DamagePerTick(0.0f)
		, TickInterval(0.5f)
		, TimeSinceLastTick(0.0f)
	{}
};

/**
 * Attack State Enumeration
 */
UENUM(BlueprintType)
enum class EAttackState : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Attacking UMETA(DisplayName = "Attacking"),
	OnCooldown UMETA(DisplayName = "On Cooldown")
};

/**
 * Elite Action Enumeration
 */
UENUM(BlueprintType)
enum class EEliteAction : uint8
{
	Move_Towards_Player UMETA(DisplayName = "Move Towards Player"),
	Move_Away_From_Player UMETA(DisplayName = "Move Away From Player"),
	Strafe_Left UMETA(DisplayName = "Strafe Left"),
	Strafe_Right UMETA(DisplayName = "Strafe Right"),
	Primary_Attack UMETA(DisplayName = "Primary Attack"),
	Secondary_Attack UMETA(DisplayName = "Secondary Attack")
};

/**
 * Reinforcement Learning State Structure
 * All values normalized to [0,1] unless otherwise specified
 */
USTRUCT(BlueprintType)
struct FRLState
{
	GENERATED_BODY()

	// Self
	UPROPERTY(BlueprintReadOnly)
	float DistanceToPlayer; // Normalized by MaxAttackRange, clamped [0,1]

	UPROPERTY(BlueprintReadOnly)
	float SelfHealthPercentage; // [0,1]

	UPROPERTY(BlueprintReadOnly)
	float TimeSinceLastAttack; // Normalized by max time (5 seconds)

	UPROPERTY(BlueprintReadOnly)
	bool bIsBeyondMaxRange; // True if actual distance > MaxAttackRange

	UPROPERTY(BlueprintReadOnly)
	bool bTookDamageRecently; // True if damaged in last 1.0s (proxy for "player attacking")

	// Player
	UPROPERTY(BlueprintReadOnly)
	float PlayerHealthPercentage; // [0,1]

	UPROPERTY(BlueprintReadOnly)
	bool bHasLineOfSightToPlayer; // True if unobstructed

	// Allies (closest 3)
	UPROPERTY(BlueprintReadOnly)
	float HealthOfClosestAlly; // [0,1]

	UPROPERTY(BlueprintReadOnly)
	float DistanceToClosestAlly; // Normalized by 2000 units

	UPROPERTY(BlueprintReadOnly)
	float HealthOfSecondClosestAlly;

	UPROPERTY(BlueprintReadOnly)
	float DistanceToSecondClosestAlly;

	UPROPERTY(BlueprintReadOnly)
	float HealthOfThirdClosestAlly;

	UPROPERTY(BlueprintReadOnly)
	float DistanceToThirdClosestAlly;

	// Team awareness
	UPROPERTY(BlueprintReadOnly)
	float NumNearbyAllies; // Count within 1000 units, normalized by max team size (5)

	FRLState()
		: DistanceToPlayer(0.0f)
		, SelfHealthPercentage(1.0f)
		, TimeSinceLastAttack(0.0f)
		, bIsBeyondMaxRange(false)
		, bTookDamageRecently(false)
		, PlayerHealthPercentage(1.0f)
		, bHasLineOfSightToPlayer(true)
		, HealthOfClosestAlly(0.0f)
		, DistanceToClosestAlly(1.0f)
		, HealthOfSecondClosestAlly(0.0f)
		, DistanceToSecondClosestAlly(1.0f)
		, HealthOfThirdClosestAlly(0.0f)
		, DistanceToThirdClosestAlly(1.0f)
		, NumNearbyAllies(0.0f)
	{
	}
};

/**
 * Base Reinforcement Learning Component for Elite Enemies
 * Implements Q-Learning with Linear Function Approximation
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SOULSTRIKE_API URLComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// ========== INITIALIZATION ==========
	
	/** Initialize the RL component with the owning pawn */
	void Initialize(APawn* InPawn);

	/** Main RL execution step called by the AI controller */
	void ExecuteRLStep(float DeltaTime);

	// ========== RL HYPERPARAMETERS ==========

	/** Learning rate (alpha) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RL|Hyperparameters")
	float Alpha;

	/** Discount factor (gamma) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RL|Hyperparameters")
	float Gamma;

	/** Exploration rate (epsilon) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RL|Hyperparameters")
	float Epsilon;

	/** Epsilon decay rate per second (0 = no decay) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RL|Hyperparameters")
	float EpsilonDecayRate;

	// ========== DEBUG ==========

	/** Enable debug visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RL|Debug")
	bool bDebugMode;

protected:
	// ========== STATE & ACTION ==========

	/** Current state */
	FRLState CurrentState;

	/** Previous state */
	FRLState PreviousState;

	/** Last action taken */
	EEliteAction LastAction;

	/** Last reward received */
	float LastReward;

	/** Actual distance to player (non-normalized, for reward calculations) */
	float ActualDistanceToPlayer;

	/** Player location (updated by Director AI) */
	FVector CachedPlayerLocation;

	/** Reference to the player character */
	ACharacter* PlayerCharacter;

	/** Reference to the controlled character (BP_Enemy pawn) */
	UPROPERTY()
	ACharacter* OwnerCharacter;

	/** Reference to C++ Elite behavior object (for attack logic) */
	UPROPERTY()
	AEliteEnemy* EliteBehavior;

	/** Elite type for weight persistence */
	EEliteType EliteType;

	/** Q-Learning brain (handles all Q-value calculations) */
	TSharedPtr<FQLearningBrain> Brain;

public:
	// ========== ELITE STATS (accessible from AI controller) ==========

	/** Attack damage - copied from Elite behavior object */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float AttackDamage;

	/** Max attack range - copied from Elite behavior object */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxAttackRange;

	/** Attack windup duration - copied from Elite behavior object */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackWindupDuration;

	/** Attack cooldown - copied from Elite behavior object */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackCooldown;

	/** Explicit movement speed cache for debug & direct movement scaling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MovementSpeed;

protected:
	// ========== INTERNAL STATE TRACKING ==========

	/** Time since last primary attack */
	float TimeSinceLastPrimaryAttack;

	/** Time since last damage taken */
	float TimeSinceLastDamageTaken;

	/** Health at previous tick (to detect damage) */
	float PreviousHealth;

	/** Damage dealt history (last 5 seconds) - for DPS calculation */
	TArray<TPair<float, float>> DamageHistory; // (Time, Damage)

	/** Healing done history (last 5 seconds) - for healer HPS calculation */
	TArray<TPair<float, float>> HealingHistory; // (Time, HealAmount)

	/** Current action persistence counter */
	float ActionPersistenceTimer;

	/** Last selected action before persistence check */
	EEliteAction PendingAction;

	// ========== STATS POLLING ==========
	
	/** Timer for periodic stat polling (separate from other timers) */
	float StatsPollTimer;
	
	/** How often to poll Blueprint stats for changes (in seconds) */
	float StatsPollInterval;
	
	/** Update stats from Blueprint (called periodically) */
	void PollAndUpdateStats();

	// ========== ATTACK STATE MACHINE ==========

	/** Current attack state */
	EAttackState AttackState;

	/** Timer for attack duration and cooldown */
	float AttackTimer;

	/** Cached player location when attack windup started (to check range on completion) */
	FVector AttackWindupStartPlayerLocation;

	/** Target ally for healing (Healer only) */
	UPROPERTY()
	ACharacter* HealTarget;

	// ========== DELTA TRACKING ==========

	/** Previous DPS for delta calculation */
	float PreviousDPS;

	/** Previous HPS for delta calculation */
	float PreviousHPS;

	/** Previous distance to player for delta calculation */
	float PreviousDistanceToPlayer;

	// ========== POISON TRACKING (for Assassin) ==========
	
	/** Active poison effects (for Assassin elite type) */
	TArray<FPoisonEffect> ActivePoisons;

	/** Update poison ticks (called every frame for Assassin) - VIRTUAL for override */
	virtual void UpdatePoisons(float DeltaTime);

	/** Check if poison is active on player (for Assassin reward logic) */
	bool HasActivePoisonOnPlayer() const;

public:
	/** Record damage dealt to player */
	void RecordDamageDealt(float Damage);

	/** Record healing done to ally */
	void RecordHealingDone(float HealAmount);

	/** Get average DPS over the last 5 seconds */
	float GetAverageDPS() const;

	/** Get average healing per second over the last 5 seconds */
	float GetAverageHPS() const;

	/** Check if can perform attack (not on cooldown or attacking) */
	bool CanAttack() const;

	/** Check if just attacked recently */
	bool JustAttacked() const;

	/** Find the N closest allies (made public for Healer secondary attack) */
	void FindClosestAllies(TArray<ACharacter*>& OutAllies, int32 NumAllies = 3);

	/** How long to stick with an action before allowing change (smoother movement) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RL|Movement")
	float MinActionDuration;

protected:
	/** Clean up old damage/healing records */
	void CleanupDamageHistory(float CurrentTime);

	// ========== CORE RL METHODS ==========

	/** Build the current state from world data */
	FRLState BuildState();

	/** Execute the chosen action in the world */
	void ExecuteAction(EEliteAction Action, float DeltaTime);

	/** Calculate the reward for the current state (VIRTUAL - override in subclasses) */
	virtual float CalculateReward();

	// ========== HELPER METHODS ==========

	/** Check if this elite has line of sight to the player */
	bool HasLineOfSightToPlayer();

	/** Count nearby allies within a radius */
	int32 CountNearbyAllies(float Radius = 1000.0f);

	/** Callback when player position is updated */
	UFUNCTION()
	void OnPlayerPositionUpdated(const FVector& NewPlayerPosition);

	/** Debug visualization */
	void DebugDraw();

	/** Get health percentage from Blueprint variables */
	float GetCharacterHealthPercentage(ACharacter* Character) const;

	/** Check if character is alive */
	bool IsCharacterAlive(ACharacter* Character) const;

	/** Call attack on the C++ Elite behavior object */
	void PerformPrimaryAttackOnElite();
	virtual void PerformSecondaryAttackOnElite();

	/** Called when attack windup completes - applies damage if player still in range - VIRTUAL for override */
	virtual void OnAttackWindupComplete();
};
