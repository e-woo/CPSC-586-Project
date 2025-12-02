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
	PrimaryActorTick.bCanEverTick = true; // for packaged compatibility

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
		// Determine which RL component to create based on pawn name (works with BP_Enemy parent)
		FString PawnName = InPawn->GetName();
		
		if (PawnName.Contains("Archer"))
		{
			RLComponent = NewObject<UArcherRLComponent>(this, UArcherRLComponent::StaticClass(), TEXT("ArcherRLComponent"));
		}
		else if (PawnName.Contains("Assassin"))
		{
			RLComponent = NewObject<UAssassinRLComponent>(this, UAssassinRLComponent::StaticClass(), TEXT("AssassinRLComponent"));
		}
		else if (PawnName.Contains("Paladin"))
		{
			RLComponent = NewObject<UPaladinRLComponent>(this, UPaladinRLComponent::StaticClass(), TEXT("PaladinRLComponent"));
		}
		else if (PawnName.Contains("Giant"))
		{
			RLComponent = NewObject<UGiantRLComponent>(this, UGiantRLComponent::StaticClass(), TEXT("GiantRLComponent"));
		}
		else if (PawnName.Contains("Healer"))
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
			UE_LOG(LogTemp, Log, TEXT("EliteAIController: Created RL component type: %s for %s"), 
				*RLComponent->GetClass()->GetName(), *PawnName);
		}
	}

	if (RLComponent)
	{
		// Initial hyperparameter sync only (they remain constant unless designer changes in editor)
		RLComponent->Alpha = LearningRate;
		RLComponent->Gamma = DiscountFactor;
		RLComponent->Epsilon = ExplorationRate; // starting epsilon
		RLComponent->EpsilonDecayRate = ExplorationDecayRate;
		RLComponent->MinActionDuration = ActionPersistenceDuration;
		RLComponent->bDebugMode = bEnableDebugMode;
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

	// Only update values that may change at runtime (decay rate, persistence, debug flag)
	RLComponent->EpsilonDecayRate = ExplorationDecayRate;
	RLComponent->MinActionDuration = ActionPersistenceDuration;
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
