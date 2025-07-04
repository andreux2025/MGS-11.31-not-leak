#pragma once
#include "Utils.h"
#include "Gamemode.h"
#include "PC.h"
#include "Tick.h"
#include "Building.h"
#include "InventoryBots.h"
#include "minhook/MinHook.h"
#include "Vehicles.h"

#pragma comment(lib, "minhook/minhook.lib")

static auto ImageBase = InSDKUtils::GetImageBase();

int True() {
	return 1;
}

int False() {
	return 0;
}


namespace Gamemode {

	inline void InitHooks()
	{
		Log("Gamemode Hooks");

		MH_CreateHook((LPVOID)(ImageBase + 0x1540390), ReadyToStartMatch, (LPVOID*)&ReadyToStartMatchOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x154A770), SpawnDefaultPawnFor, nullptr);
		MH_CreateHook(LPVOID(ImageBase + 0x153B210), PickTeam, nullptr);
		MH_CreateHook((LPVOID)(ImageBase + 0x19A0480), HandleStartingNewPlayer, (LPVOID*)&HandleStartingNewPlayerOG);
		MH_CreateHook((LPVOID)(ImageBase + Offsets::ProcessEvent), PE, (LPVOID*)&PEOG);
	}
}

namespace Misc {

	void InitHooks()
	{
		Log("Misc Hooks");

		MH_CreateHook((LPVOID)(ImageBase + 0x27C47A0), True, nullptr); // collect garbage
		MH_CreateHook((LPVOID)(ImageBase + 0x372FB80), False, nullptr);// validation / kick
		MH_CreateHook((LPVOID)(ImageBase + 0x19B1660), False, nullptr);// change gamesession id
		MH_CreateHook((LPVOID)(ImageBase + 0xE2BF70), DispatchRequest, (LPVOID*)&DispatchRequestOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x34AF6C0), AActorGetNetMode, nullptr);
		MH_CreateHook((LPVOID)(ImageBase + 0x3B76FE0), WorldGetNetMode, nullptr);
		MH_CreateHook((LPVOID)(ImageBase + 0x174AAA0), SpawnLoot, nullptr);
		MH_CreateHook((LPVOID)(ImageBase + 0x1551340), Storm, (LPVOID*)&StormOG); 
		MH_CreateHook((LPVOID)(ImageBase + 0x154e080), StartAircraftPhaseHook, (LPVOID*)&StartAircraftPhase);

		for (size_t i = 0; i < UObject::GObjects->Num(); i++)
		{
			UObject* Obj = UObject::GObjects->GetByIndex(i);
			if (Obj && Obj->IsA(AFortPhysicsPawn::StaticClass()))
			{
				HookVTable(Obj->Class->DefaultObject, 0xED, ServerMove, nullptr);
			}
		}
	}
}

namespace PC {

	void InitHooks()
	{
		Log("PC Hooks");

		MH_CreateHook((LPVOID)(ImageBase + 0x1E3D9E0), ServerLoadingScreenDropped, (LPVOID*)&ServerLoadingScreenDroppedOG); // i cant find the index

		HookVTable(UFortControllerComponent_Aircraft::GetDefaultObj(), 0x82, ServerAttemptAircraftJump, nullptr);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x10A, ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x269, ServerReadyToStartMatch, (LPVOID*)&ServerReadyToStartMatchOG);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x265, ServerReturnToMainMenu, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x184E490), ServerAttemptInteract, (LPVOID*)&ServerAttemptInteractOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x2446300), ClientOnPawnDied, (LPVOID*)&ClientOnPawnDiedOG);

		HookVTable(AFortPlayerStateAthena::StaticClass()->DefaultObject, 0xFC, ServerSetInAircraft, (LPVOID*)&ServerSetInAircraftOG);

	}
}

namespace Tick {

	void InitHooks()
	{
		Log("Tick Hooks");

		MH_CreateHook((LPVOID)(ImageBase + 0x3883CD0), TickFlush, (LPVOID*)&TickFlushOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x3b24510), GetMaxTickRate, nullptr);
	}
}

namespace Abilities {

	void InitHooks()
	{
		Log("Abilities Hooks");

		HookVTable(UAbilitySystemComponent::GetDefaultObj(), 0xF7, InternalServerTryActivateAbilityHook, nullptr);
		HookVTable(UFortAbilitySystemComponent::GetDefaultObj(), 0xF7, InternalServerTryActivateAbilityHook, nullptr);
		HookVTable(UFortAbilitySystemComponentAthena::GetDefaultObj(), 0xF7, InternalServerTryActivateAbilityHook, nullptr);

		HookVTable(APlayerPawn_Athena_C::GetDefaultObj(), 0x1E1, ServerSendZiplineState, nullptr);
	}
}

namespace Inventory {

	void InitHooks()
	{
		Log("Inventory Hooks");

		MH_CreateHook((LPVOID)(ImageBase + 0x210CC00), OnReload, (LPVOID*)&OnReloadOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x12A7810), InventoryBot, (LPVOID*)&InventoryBotOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x23F7310), NetMulticast_BatchedDamageCues, (LPVOID*)&NetMulticast_BatchedDamageCuesOG);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x21D, ServerAttemptInventoryDrop, nullptr);

		HookVTable(APlayerPawn_Athena_C::GetDefaultObj(), 0x1D6, ServerHandlePickup, (LPVOID*)&ServerHandlePickupOG);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x209, ServerExecuteInventoryItem, nullptr);
	}
}

namespace Building {

	void InitHooks()
	{
		Log("Building Hooks");

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x230, ServerCreateBuildingActor, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x237, ServerBeginEditingBuildingActor, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x235, ServerEndEditingBuildingActor, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x232, ServerEditBuildingActor, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x22C, ServerRepairBuildingActor, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x2176A20), OnDamageServer, (LPVOID*)&OnDamageServerOG);

	}
}