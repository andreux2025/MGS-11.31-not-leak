#pragma once
#include "Utils.h"

inline char (*InventoryBotOG)(AActor* a1, __int64 a2);
inline char __fastcall InventoryBot(AActor* a1, __int64 a2)
{
	*(AActor**)(__int64(a1) + 0x488) = SpawnActor<AFortInventory>({}, {}, a1);
	return InventoryBotOG(a1, a2);
}

void GiveItem(ABP_PhoebePlayerController_C* PC, UFortItemDefinition* Def, int Count = 1, int LoadedAmmo = 0)
{
	UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);
	Item->OwnerInventory = PC->Inventory;
	Item->ItemEntry.LoadedAmmo = LoadedAmmo;
	PC->Inventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
	PC->Inventory->Inventory.ItemInstances.Add(Item);
	PC->Inventory->Inventory.MarkItemDirty(Item->ItemEntry);
	PC->Inventory->HandleInventoryLocalUpdate();
}

void EquipPickaxe(AFortPlayerPawnAthena* Pawn, ABP_PhoebePlayerController_C* PC)
{
	
	if (!Pawn || !Pawn->CurrentWeapon)
		return;

	if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
	{
		for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
			{
				Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition, PC->Inventory->Inventory.ReplicatedEntries[i].ItemGuid);
				break;
			}
		}
	}
}

FFortItemEntry* GetEntry(ABP_PhoebePlayerController_C* PC, UFortItemDefinition* Def)
{
	for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
			return &PC->Inventory->Inventory.ReplicatedEntries[i];
	}

	return nullptr;
}