#pragma once
#include "Utils.h"
#include "Abilities.h"
#include "Looting.h"
#include "Inventory.h"
#include "Vehicles.h"
#include <thread>

void (*ServerReadyToStartMatchOG)(AFortPlayerControllerAthena* PC);
void ServerReadyToStartMatch(AFortPlayerControllerAthena* PC)
{
	if (!PC) {
		return;
	}

	AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

	AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

	float CurrentTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
	float MaxEndTime = GameState->WarmupCountdownStartTime + 120.f; // Max: 2 minutes
	float DesiredExtension = 10.f;

	GameState->MapInfo->SupplyDropInfoList[0]->SupplyDropClass = StaticLoadObject<UClass>("/Game/Athena/SupplyDrops/AthenaSupplyDrop_Holiday.AthenaSupplyDrop_Holiday_C");

	GameState->DefaultBattleBus = StaticLoadObject<UAthenaBattleBusItemDefinition>("/Game/Athena/Items/Cosmetics/BattleBuses/BBID_WinterBus.BBID_WinterBus");

	for (size_t i = 0; i < GameMode->BattleBusCosmetics.Num(); i++)
	{
		GameMode->BattleBusCosmetics[i] = GameState->DefaultBattleBus;
	}

	static bool areyoureal = false;
	if (!areyoureal)
	{
		areyoureal = true;
		SpawnFloorLoot();
		SpawnLlamas();
		SpawnVehicles();

		auto TS = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
		auto DR = 60.f;

		GameState->WarmupCountdownEndTime = TS + DR;
		GameMode->WarmupCountdownDuration = DR;
		GameState->WarmupCountdownStartTime = TS;
		GameMode->WarmupEarlyCountdownDuration = DR;

	}

	if (GameState->GamePhase == EAthenaGamePhase::Warmup)
	{
		if (GameState->WarmupCountdownEndTime < MaxEndTime)
		{
			float NewEndTime = GameState->WarmupCountdownEndTime + DesiredExtension;

			if (NewEndTime > MaxEndTime)
				NewEndTime = MaxEndTime;

			GameState->WarmupCountdownEndTime = NewEndTime;
			GameMode->WarmupCountdownDuration = NewEndTime - GameState->WarmupCountdownStartTime;
		}
	}

	return ServerReadyToStartMatchOG(PC);
}

inline void (*ServerLoadingScreenDroppedOG)(AFortPlayerControllerAthena* PC);
inline void ServerLoadingScreenDropped(AFortPlayerControllerAthena* PC)
{
	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
	AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
	AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	auto Pawn = (AFortPlayerPawn*)PC->Pawn;

	UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerState);
	PlayerState->OnRep_CharacterData();

	InitAbilitiesForPlayer(PC);

	((AFortPlayerStateAthena*)PC->PlayerState)->SquadId = ((AFortPlayerStateAthena*)PC->PlayerState)->TeamIndex - 2;
	((AFortPlayerStateAthena*)PC->PlayerState)->OnRep_TeamIndex(0);
	((AFortPlayerStateAthena*)PC->PlayerState)->OnRep_SquadId();
	((AFortPlayerStateAthena*)PC->PlayerState)->OnRep_PlayerTeam();
	((AFortPlayerStateAthena*)PC->PlayerState)->OnRep_PlayerTeamPrivate();

	UAthenaPickaxeItemDefinition* PickaxeItemDef;
	FFortAthenaLoadout& CosmecticLoadoutPC = PC->CosmeticLoadoutPC;

	PickaxeItemDef = CosmecticLoadoutPC.Pickaxe != nullptr ? CosmecticLoadoutPC.Pickaxe : UObject::FindObject<UAthenaPickaxeItemDefinition>("DefaultPickaxe");// just incase the backend yk

	GiveItem(PC, PC->CosmeticLoadoutPC.Pickaxe->WeaponDefinition, 1, 0);

	Pawn->CosmeticLoadout = PC->CosmeticLoadoutPC;
	Pawn->OnRep_CosmeticLoadout();

	for (size_t i = 0; i < GameMode->StartingItems.Num(); i++)
	{
		if (GameMode->StartingItems[i].Count > 0)
		{
			GiveItem(PC, GameMode->StartingItems[i].Item, GameMode->StartingItems[i].Count, 0);
		}
	}

	FGameMemberInfo Info{ -1,-1,-1 };
	Info.MemberUniqueId = PlayerState->UniqueId;
	Info.SquadId = PlayerState->SquadId;
	Info.TeamIndex = PlayerState->TeamIndex;

	GameState->GameMemberInfoArray.Members.Add(Info);
	GameState->GameMemberInfoArray.MarkItemDirty(Info);

	PC->XPComponent->bRegisteredWithQuestManager = true;
	PC->XPComponent->OnRep_bRegisteredWithQuestManager();

	return ServerLoadingScreenDroppedOG(PC);
}

inline void ServerAttemptAircraftJump(UFortControllerComponent_Aircraft* Comp, FRotator Rotation)
{
	auto PC = (AFortPlayerControllerAthena*)Comp->GetOwner();
	auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	auto Pawn = (AFortPlayerPawn*)PC->Pawn;


	if (!LateGame)
	{
		GameMode->RestartPlayer(PC);
	}

	PC->MyFortPawn->BeginSkydiving(true);
	PC->MyFortPawn->SetHealth(100);

}

void (*ServerAcknowledgePossessionOG)(AFortPlayerControllerAthena* PC, APawn* Pawn);
inline void ServerAcknowledgePossession(AFortPlayerControllerAthena* PC, APawn* Pawn)
{
	PC->AcknowledgedPawn = Pawn;
	return ServerAcknowledgePossessionOG(PC, Pawn);
}

inline void ServerExecuteInventoryItem(AFortPlayerControllerAthena* PC, FGuid Guid)
{
	if (!PC->MyFortPawn || !PC->Pawn)
		return;

	for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid == Guid)
		{
			UFortWeaponItemDefinition* DefToEquip = (UFortWeaponItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition;
			if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortGadgetItemDefinition::StaticClass()))
			{
				DefToEquip = ((UFortGadgetItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->GetWeaponItemDefinition();
			}
			else if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortDecoItemDefinition::StaticClass())) {
				auto DecoItemDefinition = (UFortDecoItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition;
				PC->MyFortPawn->PickUpActor(nullptr, DecoItemDefinition);
				PC->MyFortPawn->CurrentWeapon->ItemEntryGuid = Guid;
				static auto FortDecoTool_ContextTrapStaticClass = StaticLoadObject<UClass>("/Script/FortniteGame.FortDecoTool_ContextTrap");
				if (PC->MyFortPawn->CurrentWeapon->IsA(FortDecoTool_ContextTrapStaticClass))
				{
					reinterpret_cast<AFortDecoTool_ContextTrap*>(PC->MyFortPawn->CurrentWeapon)->ContextTrapItemDefinition = (UFortContextTrapItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition;
				}
				return;
			}
			PC->MyFortPawn->EquipWeaponDefinition(DefToEquip, Guid);
			break;
		}
	}
}

inline void (*ServerAttemptInteractOG)(UFortControllerComponent_Interaction* Comp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalData, EInteractionBeingAttempted InteractionBeingAttempted);
inline void ServerAttemptInteract(UFortControllerComponent_Interaction* Comp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalData, EInteractionBeingAttempted InteractionBeingAttempted)
{
	ServerAttemptInteractOG(Comp, ReceivingActor, InteractComponent, InteractType, OptionalData, InteractionBeingAttempted);

	std::cout << "ReceivingActor: " << ReceivingActor->GetFullName() << '\n';
	AFortPlayerControllerAthena* PC = Cast<AFortPlayerControllerAthena>(Comp->GetOwner());

	if (ReceivingActor->IsA(AFortAthenaSupplyDrop::StaticClass()))
	{
		if (ReceivingActor->GetName().starts_with("AthenaSupplyDrop_Llama_C_"))
		{
			static auto Drop = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaLlama");

			auto LootDrops = PickLootDrops(Drop);

			auto CorrectLocation = ReceivingActor->K2_GetActorLocation();

			for (auto& LootDrop : LootDrops)
			{
				SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, EFortPickupSourceTypeFlag::Container, EFortPickupSpawnSource::SupplyDrop);
			}
		}
		else
		{
			static auto Drop = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaSupplyDrop");

			auto LootDrops = PickLootDrops(Drop);

			auto CorrectLocation = ReceivingActor->K2_GetActorLocation();

			for (auto& LootDrop : LootDrops)
			{
				SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, EFortPickupSourceTypeFlag::Container, EFortPickupSpawnSource::SupplyDrop);
			}
		}
	}
	else if (PC->MyFortPawn && PC->MyFortPawn->IsInVehicle())
	{
		auto Vehicle = PC->MyFortPawn->GetVehicle();

		if (Vehicle)
		{
			auto SeatIdx = PC->MyFortPawn->GetVehicleSeatIndex();
			auto WeaponComp = Vehicle->GetSeatWeaponComponent(SeatIdx);
			if (WeaponComp)
			{
				GiveItem(PC, WeaponComp->WeaponSeatDefinitions[SeatIdx].VehicleWeapon, 1, 9999);
				for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition == WeaponComp->WeaponSeatDefinitions[SeatIdx].VehicleWeapon)
					{
						PC->SwappingItemDefinition = PC->MyFortPawn->CurrentWeapon->WeaponData;
						PC->ServerExecuteInventoryItem(PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid);
						break;
					}
				}
			}
		}
	}
}

inline void (*ServerHandlePickupOG)(AFortPlayerPawn* Pawn, AFortPickup* Pickup, float InFlyTime, FVector InStartDirection, bool bPlayPickupSound);
inline void ServerHandlePickup(AFortPlayerPawnAthena* Pawn, AFortPickup* Pickup, float InFlyTime, const FVector& InStartDirection, bool bPlayPickupSound)
{
	if (!Pickup || !Pawn || !Pawn->Controller || Pickup->bPickedUp)
		return;

	AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;

	UFortItemDefinition* Def = Pickup->PrimaryPickupItemEntry.ItemDefinition;
	FFortItemEntry* FoundEntry = nullptr;
	FFortItemEntry& PickupEntry = Pickup->PrimaryPickupItemEntry;
	float MaxStackSize = GetMaxStack(Def);
	bool Stackable = Def->IsStackable();
	UFortItemDefinition* PickupItemDef = PickupEntry.ItemDefinition;
	bool Found = false;
	FFortItemEntry* GaveEntry = nullptr;

	if (IsInventoryFull(PC))
	{
		if (Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) || Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortTrapItemDefinition::StaticClass()))
		{
			GiveItemStack(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);

			Pickup->PickupLocationData.bPlayPickupSound = true;
			Pickup->PickupLocationData.FlyTime = 0.3f;
			Pickup->PickupLocationData.ItemOwner = Pawn;
			Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
			Pickup->PickupLocationData.PickupTarget = Pawn;
			Pickup->OnRep_PickupLocationData();

			Pickup->bPickedUp = true;
			Pickup->OnRep_bPickedUp();
			return;
		}

		if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
		{
			if (Stackable)
			{
				for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];

					if (Entry.ItemDefinition == PickupItemDef)
					{
						Found = true;
						if ((MaxStackSize - Entry.Count) > 0)
						{
							Entry.Count += PickupEntry.Count;

							if (Entry.Count > MaxStackSize)
							{
								SpawnStack((APlayerPawn_Athena_C*)PC->Pawn, PickupItemDef, Entry.Count - MaxStackSize);
								Entry.Count = MaxStackSize;
							}

							PC->WorldInventory->Inventory.MarkItemDirty(Entry);
						}
						else
						{
							if (IsPrimaryQuickbar(PickupItemDef))
							{
								GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count);
							}
						}
						break;
					}
				}
				if (!Found)
				{
					for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
					{
						if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid == Pawn->CurrentWeapon->GetInventoryGUID())
						{
							PC->ServerAttemptInventoryDrop(Pawn->CurrentWeapon->GetInventoryGUID(), PC->WorldInventory->Inventory.ReplicatedEntries[i].Count, false);
							break;
						}
					}
					GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count, false, 0, true);
				}

				Pickup->PickupLocationData.bPlayPickupSound = true;
				Pickup->PickupLocationData.FlyTime = 0.3f;
				Pickup->PickupLocationData.ItemOwner = Pawn;
				Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
				Pickup->PickupLocationData.PickupTarget = Pawn;
				Pickup->OnRep_PickupLocationData();

				Pickup->bPickedUp = true;
				Pickup->OnRep_bPickedUp();
				return;
			}

			for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
			{
				if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid == Pawn->CurrentWeapon->GetInventoryGUID())
				{
					PC->ServerAttemptInventoryDrop(Pawn->CurrentWeapon->GetInventoryGUID(), PC->WorldInventory->Inventory.ReplicatedEntries[i].Count, false);
					break;
				}
			}
		}
	}

	if (!IsInventoryFull(PC))
	{
		if (Stackable && !Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Stackable && !Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) || Stackable && Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortTrapItemDefinition::StaticClass()))
		{
			for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
			{
				FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];

				if (Entry.ItemDefinition == PickupItemDef)
				{
					Found = true;
					if ((MaxStackSize - Entry.Count) > 0)
					{
						Entry.Count += PickupEntry.Count;

						if (Entry.Count > MaxStackSize)
						{
							SpawnStack((APlayerPawn_Athena_C*)PC->Pawn, PickupItemDef, Entry.Count - MaxStackSize);
							Entry.Count = MaxStackSize;
						}

						PC->WorldInventory->Inventory.MarkItemDirty(Entry);
					}
					else
					{
						if (IsPrimaryQuickbar(PickupItemDef))
						{
							GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count);
						}
					}
					break;
				}
			}
			if (!Found)
			{
				GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count, false, 0, true);
			}

			Pickup->PickupLocationData.bPlayPickupSound = true;
			Pickup->PickupLocationData.FlyTime = 0.3f;
			Pickup->PickupLocationData.ItemOwner = Pawn;
			Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
			Pickup->PickupLocationData.PickupTarget = Pawn;
			Pickup->OnRep_PickupLocationData();

			Pickup->bPickedUp = true;
			Pickup->OnRep_bPickedUp();
			return;
		}

		if (Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) || Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortTrapItemDefinition::StaticClass()))
		{
			GiveItemStack(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);
		}
		else {
			GiveItem(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);
		}
	}

	Pickup->PickupLocationData.bPlayPickupSound = true;
	Pickup->PickupLocationData.FlyTime = 0.3f;
	Pickup->PickupLocationData.ItemOwner = Pawn;
	Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
	Pickup->PickupLocationData.PickupTarget = Pawn;
	Pickup->OnRep_PickupLocationData();

	Pickup->bPickedUp = true;
	Pickup->OnRep_bPickedUp();
}

inline void ServerAttemptInventoryDrop(AFortPlayerControllerAthena* PC, FGuid ItemGuid, int Count, bool bTrash)
{
	FFortItemEntry* Entry = FindEntry(PC, ItemGuid);
	SpawnPickup(Entry->ItemDefinition, Count, Entry->LoadedAmmo, PC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset);
	RemoveItem(PC, ItemGuid, Count);
}

inline void (*ClientOnPawnDiedOG)(AFortPlayerControllerAthena*, FFortPlayerDeathReport);
inline void ClientOnPawnDied(AFortPlayerControllerAthena* DeadPC, FFortPlayerDeathReport DeathReport)
{

	Log("Pawn died");
	DeadPC->bMarkedAlive = false;

	auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	AFortPlayerStateAthena* DeadState = (AFortPlayerStateAthena*)DeadPC->PlayerState;
	AFortPlayerPawnAthena* KillerPawn = (AFortPlayerPawnAthena*)DeathReport.KillerPawn;
	AFortPlayerStateAthena* KillerState = (AFortPlayerStateAthena*)DeathReport.KillerPlayerState;
	static bool Won = false;

	if (!GameState->IsRespawningAllowed(DeadState))
	{
		if (DeadPC && DeadPC->WorldInventory)
		{
			for (size_t i = 0; i < DeadPC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
			{
				if (((UFortWorldItemDefinition*)DeadPC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->bCanBeDropped)
				{
					SpawnPickup(DeadPC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.ItemDefinition, DeadPC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.Count, DeadPC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.LoadedAmmo, DeadPC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination, DeadPC->MyFortPawn);
				}
			}
		}
	}

	if (!Won && DeadPC && DeadState)
	{
		if (KillerState && KillerState != DeadState)
		{
			KillerState->KillScore++;

			for (size_t i = 0; i < KillerState->PlayerTeam->TeamMembers.Num(); i++)
			{
				((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->TeamKillScore++;
				((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->OnRep_TeamKillScore();
			}


			KillerState->ClientReportKill(DeadState);
			KillerState->OnRep_Kills();

			//if (SpawnedBots.empty())
			//{
				//GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, GetDefFromEvent(EAccoladeEvent::Kill, KillerState->KillScore));
			//}

			DeadState->PawnDeathLocation = DeadPC->Pawn->K2_GetActorLocation();
			FDeathInfo& DeathInfo = DeadState->DeathInfo;

			DeathInfo.bDBNO = DeadPC->MyFortPawn->bWasDBNOOnDeath;
			DeathInfo.bInitialized = true;
			DeathInfo.DeathLocation = DeadPC->Pawn->K2_GetActorLocation();
			DeathInfo.DeathTags = DeathReport.Tags;
			DeathInfo.Downer = KillerState;
			DeathInfo.Distance = (KillerPawn ? KillerPawn->GetDistanceTo(DeadPC->Pawn) : ((AFortPlayerPawnAthena*)DeadPC->Pawn)->LastFallDistance);
			DeathInfo.FinisherOrDowner = KillerState;
			DeathInfo.DeathCause = DeadState->ToDeathCause(DeathInfo.DeathTags, DeathInfo.bDBNO);
			DeadState->OnRep_DeathInfo();
		}

		if (Won || !GameState->IsRespawningAllowed(DeadState))
		{
			FAthenaRewardResult Result;
			UFortPlayerControllerAthenaXPComponent* XPComponent = DeadPC->XPComponent;
			Result.TotalBookXpGained = XPComponent->TotalXpEarned;
			Result.TotalSeasonXpGained = XPComponent->TotalXpEarned;

			DeadPC->ClientSendEndBattleRoyaleMatchForPlayer(true, Result);

			FAthenaMatchStats Stats;
			FAthenaMatchTeamStats TeamStats;

			if (DeadState)
			{
				DeadState->Place = GameMode->AliveBots.Num() + GameMode->AlivePlayers.Num();
				DeadState->OnRep_Place();
			}

			for (size_t i = 0; i < 20; i++)
			{
				Stats.Stats[i] = 0;
			}

			Stats.Stats[3] = DeadState->KillScore;

			TeamStats.Place = DeadState->Place;
			TeamStats.TotalPlayers = GameState->TotalPlayers;

			DeadPC->ClientSendMatchStatsForPlayer(Stats);
			DeadPC->ClientSendTeamStatsForPlayer(TeamStats);
			FDeathInfo& DeathInfo = DeadState->DeathInfo;

			RemoveFromAlivePlayers(GameMode, DeadPC, (KillerState == DeadState ? nullptr : KillerState), KillerPawn, DeathReport.KillerWeapon ? DeathReport.KillerWeapon : nullptr, DeadState ? DeathInfo.DeathCause : EDeathCause::Rifle, 0);

			/*if (SpawnedBots.empty())
			{
				AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
				if (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num() == 50)
				{
					for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
					{
						GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_026_Survival_Default_Bronze.AccoladeId_026_Survival_Default_Bronze"));
					}
				}
				if (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num() == 25)
				{
					for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
					{
						GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_027_Survival_Default_Silver.AccoladeId_027_Survival_Default_Silver"));
					}
				}
				if (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num() == 10)
				{
					for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
					{
						GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_028_Survival_Default_Gold.AccoladeId_028_Survival_Default_Gold"));
					}
				}
			}*/

			if (KillerState)
			{
				if (KillerState->Place == 1)
				{
					if (DeathReport.KillerWeapon)
					{
						((AFortPlayerControllerAthena*)KillerState->Owner)->PlayWinEffects(KillerPawn, DeathReport.KillerWeapon, EDeathCause::Rifle, false);
						((AFortPlayerControllerAthena*)KillerState->Owner)->ClientNotifyWon(KillerPawn, DeathReport.KillerWeapon, EDeathCause::Rifle);
					}

					FAthenaRewardResult Result;
					AFortPlayerControllerAthena* KillerPC = (AFortPlayerControllerAthena*)KillerState->GetOwner();
					KillerPC->ClientSendEndBattleRoyaleMatchForPlayer(true, Result);

					FAthenaMatchStats Stats;
					FAthenaMatchTeamStats TeamStats;

					for (size_t i = 0; i < 20; i++)
					{
						Stats.Stats[i] = 0;
					}

					Stats.Stats[3] = KillerState->KillScore;

					TeamStats.Place = 1;
					TeamStats.TotalPlayers = GameState->TotalPlayers;

					KillerPC->ClientSendMatchStatsForPlayer(Stats);
					KillerPC->ClientSendTeamStatsForPlayer(TeamStats);

					GameState->WinningPlayerState = KillerState;
					GameState->WinningTeam = KillerState->TeamIndex;
					GameState->OnRep_WinningPlayerState();
					GameState->OnRep_WinningTeam();
				}
			}
		}
	}

	return ClientOnPawnDiedOG(DeadPC, DeathReport);
}

void ServerReturnToMainMenu(AFortPlayerControllerAthena* PC) 
{
	PC->ClientReturnToMainMenu(L"");
}

static inline void(*ServerSetInAircraftOG)(AFortPlayerStateAthena* PlayerState, bool bNewInAircraft);
void ServerSetInAircraft(AFortPlayerStateAthena* PlayerState, bool bNewInAircraft)
{

	AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)PlayerState->Owner;

	if (PC && PC->WorldInventory)
	{
		for (int i = PC->WorldInventory->Inventory.ReplicatedEntries.Num() - 1; i >= 0; i--)
		{
			if (((UFortWorldItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->bCanBeDropped && !GivenLootPlayers.Contains(PC) && LateGame)
			{
				int Count = PC->WorldInventory->Inventory.ReplicatedEntries[i].Count;
				RemoveItem(PC, PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Count);
			}
			else if (((UFortWorldItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->bCanBeDropped && !LateGame)
			{
				int Count = PC->WorldInventory->Inventory.ReplicatedEntries[i].Count;
				RemoveItem(PC, PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Count);
			}
		}
	}

	if (LateGame)
	{
		auto GameState = (AFortGameStateAthena*)UEngine::GetEngine()->GameViewport->World->GameState;
		auto GameMode = (AFortGameModeAthena*)UEngine::GetEngine()->GameViewport->World->AuthorityGameMode;
		FVector BattleBusLocation = GameMode->SafeZoneLocations[3];
		BattleBusLocation.Z += 15000;
		auto Aircraft = GameState->GetAircraft(0);


		if (Aircraft)
		{
			//Aircraft->FlightInfo.FlightSpeed = ;
			Aircraft->FlightInfo.FlightStartLocation = FVector_NetQuantize100(BattleBusLocation);
			Aircraft->ExitLocation = BattleBusLocation;
			GameState->bAircraftIsLocked = true;
		}

		if (!GivenLootPlayers.Contains(PC))
		{
			GiveLoadout(PC);
			GivenLootPlayers.Add(PC);
		}

		std::thread(LateGameAircraftThread, BattleBusLocation).detach();

	}

	return ServerSetInAircraftOG(PlayerState, bNewInAircraft);
}