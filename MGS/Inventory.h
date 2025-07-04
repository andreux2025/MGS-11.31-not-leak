#pragma once
#include "Utils.h"
#include <numeric>

EFortQuickBars GetQuickBars(UFortItemDefinition* ItemDefinition)
{
	if (!ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortEditToolItemDefinition::StaticClass()) &&
		!ItemDefinition->IsA(UFortBuildingItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortTrapItemDefinition::StaticClass()) && !ItemDefinition->IsA(AFortWeaponRangedForVehicle::StaticClass()))
		return EFortQuickBars::Primary;

	return EFortQuickBars::Secondary;
}

bool IsPrimaryQuickbar(UFortItemDefinition* Def)
{
	return Def->IsA(UFortConsumableItemDefinition::StaticClass()) || Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()) || Def->IsA(UFortGadgetItemDefinition::StaticClass());
}

bool IsInventoryFull(AFortPlayerController* PC)
{
	int ItemNb = 0;
	auto InstancesPtr = &PC->WorldInventory->Inventory.ItemInstances;
	for (int i = 0; i < InstancesPtr->Num(); i++)
	{
		if (InstancesPtr->operator[](i))
		{
			if (GetQuickBars(InstancesPtr->operator[](i)->ItemEntry.ItemDefinition) == EFortQuickBars::Primary)
			{
				ItemNb++;

				if (ItemNb >= 5)
				{
					break;
				}
			}
		}
	}

	return ItemNb >= 5;
}

inline int GetMagSize(UFortWeaponItemDefinition* Def)
{
	if (!Def)
		return 0;

	UDataTable* Table = Def->WeaponStatHandle.DataTable;
	if (!Table)
		return 0;
	int Ret = 0;
	auto& RowMap = *(TMap<FName, void*>*)(__int64(Table) + 0x30);
	for (auto& Pair : RowMap)
	{
		if (Pair.Key().ToString() == Def->WeaponStatHandle.RowName.ToString())
		{
			Ret = ((FFortRangedWeaponStats*)Pair.Value())->ClipSize;
		}
	}
	return Ret;
}

int32 GetMaxStack(UFortItemDefinition* Def)
{
	return Def->MaxStackSize; 
}

inline void UpdateInventory(AFortPlayerController* PC, FFortItemEntry& Entry)
{
	for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
	{
		if (PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.ItemGuid == Entry.ItemGuid)
		{
			PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry = Entry;
			PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
			break;
		}
	}
}

void UpdateLoadedAmmo(AFortPlayerController* PC, AFortWeapon* Weapon)
{
	for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid == Weapon->ItemEntryGuid)
		{
			PC->WorldInventory->Inventory.ReplicatedEntries[i].LoadedAmmo = Weapon->AmmoCount;
			UpdateInventory((AFortPlayerControllerAthena*)PC, PC->WorldInventory->Inventory.ReplicatedEntries[i]);
			PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
			break;
		}
	}
}

void UpdateLoadedAmmo(AFortPlayerController* PC, AFortWeapon* Weapon, int AmountToAdd)
{
	for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid == Weapon->ItemEntryGuid)
		{
			PC->WorldInventory->Inventory.ReplicatedEntries[i].LoadedAmmo += AmountToAdd;
			UpdateInventory((AFortPlayerControllerAthena*)PC, PC->WorldInventory->Inventory.ReplicatedEntries[i]);
			PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
			break;
		}
	}
}

inline void GiveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count, int LoadedAmmo)
{

	UFortWorldItem* Item = Cast<UFortWorldItem>(Def->CreateTemporaryItemInstanceBP(Count, 0));
	Item->SetOwningControllerForTemporaryItem(PC);
	Item->OwnerInventory = PC->WorldInventory;
	Item->ItemEntry.LoadedAmmo = LoadedAmmo;

	PC->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
	PC->WorldInventory->Inventory.ItemInstances.Add(Item);

	if (auto Gadget = Cast<UFortGadgetItemDefinition>(Def))
		((bool(*)(UFortGadgetItemDefinition*, IInterface*, UFortWorldItem*, uint8)) (InSDKUtils::GetImageBase() + 0x693C810))(Gadget, GetInterface<IFortInventoryOwnerInterface>(PC), Item, 1);

	PC->WorldInventory->Inventory.MarkItemDirty(Item->ItemEntry);
	PC->WorldInventory->HandleInventoryLocalUpdate();

}

void UpdateStack(AFortPlayerController* PC, bool Update, FFortItemEntry* EntryToUpdate = nullptr)
{
	PC->WorldInventory->bRequiresLocalUpdate = true;
	PC->WorldInventory->HandleInventoryLocalUpdate();
	PC->HandleWorldInventoryLocalUpdate();
	PC->ClientForceUpdateQuickbar(EFortQuickBars::Primary);
	PC->ClientForceUpdateQuickbar(EFortQuickBars::Secondary);

	if (Update)
	{

		PC->WorldInventory->Inventory.MarkItemDirty(*EntryToUpdate);
	}
	else
	{
		PC->WorldInventory->Inventory.MarkArrayDirty();
	}
}

FFortItemEntry* GiveStack(AFortPlayerControllerAthena* PC, UFortItemDefinition* Def, int Count = 1, bool GiveLoadedAmmo = false, int LoadedAmmo = 0, bool Toast = false)
{
	UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);

	Item->SetOwningControllerForTemporaryItem(PC);
	Item->OwnerInventory = PC->WorldInventory;
	Item->ItemEntry.ItemDefinition = Def;
	Item->ItemEntry.Count = Count;


	if (GiveLoadedAmmo)
	{
		Item->ItemEntry.LoadedAmmo = LoadedAmmo;
	}
	Item->ItemEntry.ReplicationKey++;

	PC->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
	PC->WorldInventory->Inventory.ItemInstances.Add(Item);

	UpdateStack(PC, false);
	return &Item->ItemEntry;
}

void GiveItemStack(AFortPlayerController* PC, UFortItemDefinition* Def, int Count, int LoadedAmmo)
{
	int OutXY = Def->MaxStackSize; 

	FFortItemEntry* Found = nullptr;
	for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
		{
			Found = &PC->WorldInventory->Inventory.ReplicatedEntries[i];
			PC->WorldInventory->Inventory.ReplicatedEntries[i].Count += Count;
			if (PC->WorldInventory->Inventory.ReplicatedEntries[i].Count > OutXY)
			{
				PC->WorldInventory->Inventory.ReplicatedEntries[i].Count = OutXY;
			}

			PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
			UpdateInventory(PC, PC->WorldInventory->Inventory.ReplicatedEntries[i]);
			PC->WorldInventory->HandleInventoryLocalUpdate();
			return;
		}
	}

	if (!Found)
	{
		GiveItem(PC, Def, Count, LoadedAmmo);
	}
}

void RemoveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count)
{
	bool Remove = false;
	FGuid guid;
	for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		auto& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];
		if (Entry.ItemDefinition == Def)
		{
			Entry.Count -= Count;
			if (Entry.Count <= 0)
			{
				PC->WorldInventory->Inventory.ReplicatedEntries[i].StateValues.Free();
				PC->WorldInventory->Inventory.ReplicatedEntries.RemoveSingle(i);
				Remove = true;
				guid = Entry.ItemGuid;
			}
			else
			{
				PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				UpdateInventory(PC, Entry);
				PC->WorldInventory->HandleInventoryLocalUpdate();
				return;
			}
			break;
		}
	}

	if (Remove)
	{
		for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
		{
			if (PC->WorldInventory->Inventory.ItemInstances[i]->GetItemGuid() == guid)
			{
				PC->WorldInventory->Inventory.ItemInstances.RemoveSingle(i);
				break;
			}
		}
	}

	PC->WorldInventory->Inventory.MarkArrayDirty();
	PC->WorldInventory->HandleInventoryLocalUpdate();
}

inline void RemoveItem(AFortPlayerController* PC, FGuid Guid, int Count)
{
	for (auto& Entry : PC->WorldInventory->Inventory.ReplicatedEntries)
	{
		if (Entry.ItemGuid == Guid)
		{
			RemoveItem(PC, Entry.ItemDefinition, Count);
			break;
		}
	}
}

inline FFortItemEntry* FindEntry(AFortPlayerController* PC, FGuid Guid)
{
	for (auto& Entry : PC->WorldInventory->Inventory.ReplicatedEntries)
	{
		if (Entry.ItemGuid == Guid)
		{
			return &Entry;
		}
	}
	return nullptr;
}

FFortItemEntry* FindItemEntry(AFortPlayerController* PC, UFortItemDefinition* ItemDef)
{
	if (!PC || !PC->WorldInventory || !ItemDef)
		return nullptr;
	for (int i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); ++i)
	{
		if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition == ItemDef)
		{
			return &PC->WorldInventory->Inventory.ReplicatedEntries[i];
		}
	}
	return nullptr;
}

__int64 (*OnReloadOG)(AFortWeapon* Weapon, int RemoveCount);
__int64 OnReload(AFortWeapon* Weapon, int RemoveCount)
{
	Log("Called onreload");
	AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

	auto GameState = (AFortGameStateAthena*)GameMode->GameState;

	auto Ret = OnReloadOG(Weapon, RemoveCount);
	auto WeaponDef = Weapon->WeaponData;
	if (!WeaponDef)
		return Ret;

	auto AmmoDef = WeaponDef->GetAmmoWorldItemDefinition_BP();
	if (!AmmoDef)
		return Ret;

	AFortPlayerPawnAthena* Pawn = (AFortPlayerPawnAthena*)Weapon->GetOwner();
	AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;
	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
	if (!PC || !PC->Pawn || !PC->IsA(AFortPlayerControllerAthena::StaticClass()) || &PC->WorldInventory->Inventory == nullptr || GameState->GamePhase >= EAthenaGamePhase::EndGame)
		return Ret;

	if (PC->bInfiniteAmmo) {
		UpdateLoadedAmmo(PC, Weapon, RemoveCount);
		return Ret;
	}

	int AmmoCount = 0;
	FFortItemEntry* FoundEntry = nullptr;
	for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];

		if (Entry.ItemDefinition == AmmoDef) {
			AmmoCount = Entry.Count;
			FoundEntry = &Entry;
			break;
		}
	}

	int AmmoToRemove = (RemoveCount < AmmoCount) ? RemoveCount : AmmoCount;

	if (AmmoToRemove > 0) {
		RemoveItem(PC, AmmoDef, AmmoToRemove);
		UpdateLoadedAmmo(PC, Weapon, AmmoToRemove);
	}

	//this is a bit bugged i need to find a better way
	if (WeaponDef == StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/Shields/Athena_Shields.Athena_Shields"))
	{
		FGameplayEffectContextHandle Handle = PlayerState->AbilitySystemComponent->MakeEffectContext();
		FGameplayTag tag{};
		static auto name = UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(TEXT("GameplayCue.Shield.PotionConsumed"));
		tag.TagName = name;

		PlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(tag, FPredictionKey(), Handle);
		PlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(tag, FPredictionKey(), Handle);
	}

	if (WeaponDef == StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/Medkit/Athena_Medkit.Athena_Medkit")) //doesnt work ;(
	{
		Log("Called medkit");
		FGameplayEffectContextHandle Handle = PlayerState->AbilitySystemComponent->MakeEffectContext();
		FGameplayTag tag{};
		static auto name = UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(TEXT("GameplayCue.Athena.Health.HealUsed"));
		tag.TagName = name;

		PlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(tag, FPredictionKey(), Handle);
		PlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(tag, FPredictionKey(), Handle);
	}

	PC->WorldInventory->bRequiresLocalUpdate = true;
	//PC->WorldInventory->Inventory.MarkItemDirty();
	PC->WorldInventory->HandleInventoryLocalUpdate();

	return Ret;
}

inline void (*NetMulticast_BatchedDamageCuesOG)(AFortPawn* Pawn, void* SharedData, void* NonSharedData);
inline void NetMulticast_BatchedDamageCues(AFortPawn* Pawn, void* SharedData, void* NonSharedData)
{
	if (!Pawn || Pawn->Controller->IsA(ABP_PhoebePlayerController_C::StaticClass()) || !NetMulticast_BatchedDamageCuesOG)
		return;

	if (Pawn->CurrentWeapon)
		UpdateLoadedAmmo((AFortPlayerController*)Pawn->Controller, ((AFortPlayerPawn*)Pawn)->CurrentWeapon);

	return NetMulticast_BatchedDamageCuesOG(Pawn, SharedData, NonSharedData);
}
