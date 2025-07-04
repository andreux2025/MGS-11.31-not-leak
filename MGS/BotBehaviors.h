#pragma once
#include "Utils.h"
#include "PlayerBots.h"

enum EBotState : uint8
{
    Warmup,
    InBus,
    SkydivingFromBus,
    Landed,
    Looting,
    MovingToZone,
    MovingAroundUnbreakableObj,
    LookingForPlayers,
    Stuck,
    MAX
};

void WalkForward(AFortPlayerPawnAthena* Pawn)
{
    Pawn->AddMovementInput(Pawn->GetActorForwardVector(), 1.f, true);
}

AActor* FindNearestPlayer(AFortPlayerPawnAthena* Pawn)
{
	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

	AActor* NearestPawn = nullptr;

	for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
	{
		if (!NearestPawn || (GameMode->AlivePlayers[i]->Pawn && GameMode->AlivePlayers[i]->Pawn->GetDistanceTo(Pawn) < NearestPawn->GetDistanceTo(Pawn)))
		{
			NearestPawn = GameMode->AlivePlayers[i]->Pawn;
		}
	}

	for (size_t i = 0; i < GameMode->AliveBots.Num(); i++)
	{
		if (GameMode->AliveBots[i]->Pawn != Pawn)
		{
			if (!NearestPawn || (GameMode->AliveBots[i]->Pawn && GameMode->AliveBots[i]->Pawn->GetDistanceTo(Pawn) < NearestPawn->GetDistanceTo(Pawn)))
			{
				NearestPawn = GameMode->AliveBots[i]->Pawn;
			}
		}
	}

	return NearestPawn;
}

void Run(AFortPlayerPawnAthena* Pawn , bool Running)
{
	if (!Running)
	{
		Running = true;
		for (size_t i = 0; i < Pawn->AbilitySystemComponent->ActivatableAbilities.Items.Num(); i++)
		{
			if (Pawn->AbilitySystemComponent->ActivatableAbilities.Items[i].Ability->IsA(UFortGameplayAbility_Sprint::StaticClass()))
			{
				Pawn->AbilitySystemComponent->ServerTryActivateAbility(Pawn->AbilitySystemComponent->ActivatableAbilities.Items[i].Handle, Pawn->AbilitySystemComponent->ActivatableAbilities.Items[i].InputPressed, Pawn->AbilitySystemComponent->ActivatableAbilities.Items[i].ActivationInfo.PredictionKeyWhenActivated);
				break;
			}
		}
	}

}


void GoTo(AActor* Loc, AFortPlayerPawnAthena* Pawn, ABP_PhoebePlayerController_C* PC ,float Radius = 0)
{
	PC->MoveToActor(Loc, 0, true, false, true, nullptr, true);
	Run(Pawn, false);
}