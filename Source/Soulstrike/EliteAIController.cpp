#include "EliteAIController.h"
#include "RLComponent.h"
#include "ArcherRLComponent.h"
#include "AssassinRLComponent.h"
#include "PaladinRLComponent.h"
#include "GiantRLComponent.h"
#include "HealerRLComponent.h"
#include "EliteArcher.h"
#include "EliteAssassin.h"
#include "ElitePaladin.h"
#include "EliteGiant.h"
#include "EliteHealer.h"

AEliteAIController::AEliteAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	// Don't create RLComponent here - will be created dynamically based on pawn type
	RLComponent = nullptr;

	// Default: run RL every tick (can be changed for performance)
	RLTickInterval = 0.0f;
	RLTickAccumulator = 0.0f;

	// Debug mode off by default
	bEnableDebugMode = false;

	// RL Hyperparameters (defaults - can be tuned in Blueprint)
	LearningRate = 0.5f;           // Higher learning rate for faster adaptation
	DiscountFactor = 0.95f;        // High discount for long-term planning
	ExplorationRate = 0.2f;        // 20% random exploration
	ExplorationDecayRate = 0.0f;   // No decay by default
	ActionPersistenceDuration = 0.3f; // Smooth movement
}

void AEliteAIController::BeginPlay()
{
	Super::BeginPlay();
}

void AEliteAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// If RLComponent wasn't set in Blueprint, create the correct one based on pawn type
	if (!RLComponent && InPawn)
	{
		// Determine which RL component to create based on elite type
		if (Cast<AEliteArcher>(InPawn))
		{
			RLComponent = NewObject<UArcherRLComponent>(this, UArcherRLComponent::StaticClass(), TEXT("ArcherRLComponent"));
		}
		else if (Cast<AEliteAssassin>(InPawn))
		{
			RLComponent = NewObject<UAssassinRLComponent>(this, UAssassinRLComponent::StaticClass(), TEXT("AssassinRLComponent"));
		}
		else if (Cast<AElitePaladin>(InPawn))
		{
			RLComponent = NewObject<UPaladinRLComponent>(this, UPaladinRLComponent::StaticClass(), TEXT("PaladinRLComponent"));
		}
		else if (Cast<AEliteGiant>(InPawn))
		{
			RLComponent = NewObject<UGiantRLComponent>(this, UGiantRLComponent::StaticClass(), TEXT("GiantRLComponent"));
		}
		else if (Cast<AEliteHealer>(InPawn))
		{
			RLComponent = NewObject<UHealerRLComponent>(this, UHealerRLComponent::StaticClass(), TEXT("HealerRLComponent"));
		}
		else
		{
			// Fallback to base component
			RLComponent = NewObject<URLComponent>(this, URLComponent::StaticClass(), TEXT("RLComponent"));
		}

		// Register the component
		if (RLComponent)
		{
			RLComponent->RegisterComponent();
			UE_LOG(LogTemp, Log, TEXT("EliteAIController: Created RL component type: %s"), *RLComponent->GetClass()->GetName());
		}
	}

	if (RLComponent)
	{
		// Sync hyperparameters from AI Controller to RL Component
		RLComponent->Alpha = LearningRate;
		RLComponent->Gamma = DiscountFactor;
		RLComponent->Epsilon = ExplorationRate;
		RLComponent->EpsilonDecayRate = ExplorationDecayRate;
		RLComponent->MinActionDuration = ActionPersistenceDuration;

		// Apply debug mode setting
		RLComponent->bDebugMode = bEnableDebugMode;

		// Initialize the component
		RLComponent->Initialize(InPawn);
	}
}

void AEliteAIController::OnUnPossess()
{
	Super::OnUnPossess();
}

void AEliteAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!RLComponent)
		return;

	// Sync hyperparameters from AI Controller to RL Component (in case they were changed at runtime)
	RLComponent->Alpha = LearningRate;
	RLComponent->Gamma = DiscountFactor;
	RLComponent->Epsilon = ExplorationRate;
	RLComponent->EpsilonDecayRate = ExplorationDecayRate;
	RLComponent->MinActionDuration = ActionPersistenceDuration;

	// Update debug mode in case it was changed at runtime
	if (RLComponent->bDebugMode != bEnableDebugMode)
	{
		RLComponent->bDebugMode = bEnableDebugMode;
	}

	// If RLTickInterval is 0, run every tick
	if (RLTickInterval <= 0.0f)
	{
		RLComponent->ExecuteRLStep(DeltaTime);
	}
	else
	{
		// Accumulate time and run at fixed intervals
		RLTickAccumulator += DeltaTime;
		if (RLTickAccumulator >= RLTickInterval)
		{
			RLComponent->ExecuteRLStep(RLTickAccumulator);
			RLTickAccumulator = 0.0f;
		}
	}
}
