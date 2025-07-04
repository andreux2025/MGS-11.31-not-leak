#pragma once
#include "Utils.h"
#include "PlayerBots.h"

inline void (*TickFlushOG)(UNetDriver* Driver, float DeltaSeconds);
inline void TickFlush(UNetDriver* Driver, float DeltaSeconds)
{
	static void (*ServerReplicateActors)(void*) = decltype(ServerReplicateActors)(InSDKUtils::GetImageBase() + 0xc7cc50);
	if (ServerReplicateActors && Driver->ReplicationDriver)
		ServerReplicateActors(Driver->ReplicationDriver);

	auto GameMode = (AFortGameModeAthena*)Driver->World->AuthorityGameMode;
	static TArray<AActor*> PlayerStarts;
	static bool First = false;
	static bool HasStartedAircraft = false;
	auto GameState = (AFortGameStateAthena*)Driver->World->GameState;
	auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

	if (Botlobbies && ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->GamePhase < EAthenaGamePhase::Aircraft && GameMode->AlivePlayers.Num() > 0 && (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num()) < NumPlayersToStart)
	{
		GameState->WarmupCountdownEndTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) + 10;

		if (!First)
		{
			UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &PlayerStarts);
			First = true;
		}

		if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.07f))
		{
			AActor* SpawnLocator = PlayerStarts[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, PlayerStarts.Num() - 1)];

			if (SpawnLocator)
			{
				SpawnedBots.push_back(new Bot(SpawnLocator));
			}
		}
	}

	if (Botlobbies)
	{
		for (size_t i = 0; i < SpawnedBots.size(); i++)
		{
			SpawnedBots[i]->TickBot();
		}
	}

	if (GameMode->AlivePlayers.Num() == NumPlayersToStart && GameState->GamePhase == EAthenaGamePhase::Warmup && GameState->WarmupCountdownEndTime - UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) <= 0 && HasStartedAircraft == false)
	{
		StartAircraft();

		HasStartedAircraft = true;

	}

	if (GameState->WarmupCountdownEndTime - Statics->GetTimeSeconds(UWorld::GetWorld()) <= 0 && GameState->GamePhase == EAthenaGamePhase::Warmup && HasStartedAircraft == true)
	{
		StartAircraftPhase(GameMode, 0);
	}

	return TickFlushOG(Driver, DeltaSeconds);
}

inline float GetMaxTickRate() 
{
	return 30.f;
}

__int64 AActorGetNetMode(AActor* a1)
{
	return 1;
}

__int64 WorldGetNetMode(UWorld* a1)
{
	return 1;
}
