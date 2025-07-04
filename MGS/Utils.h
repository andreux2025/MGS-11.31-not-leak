#pragma once // this is basicly my framework file 
#include <string>
#include <iostream>
#include <map>
#include "SDK.hpp"

using namespace std;
using namespace SDK;


static UNetDriver* (*CreateNetDriver)(void*, void*, FName) = decltype(CreateNetDriver)(InSDKUtils::GetImageBase() + 0x3B21C20);
static bool (*InitListen)(void*, void*, FURL&, bool, FString&) = decltype(InitListen)(InSDKUtils::GetImageBase() + 0x8efba0);
static void (*SetWorld)(void*, void*) = decltype(SetWorld)(InSDKUtils::GetImageBase() + 0x3882f50);
static UObject* (*StaticLoadObjectOG)(UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename, uint32_t LoadFlags, UObject* Sandbox, bool bAllowObjectReconciliation, void*) = decltype(StaticLoadObjectOG)(__int64(GetModuleHandleW(0)) + 0x2849140);
static UObject* (*StaticFindObjectOG)(UClass*, UObject* Package, const wchar_t* OrigInName, bool ExactClass) = decltype(StaticFindObjectOG)(__int64(GetModuleHandleW(0)) + 0x2847D60);
static FVector* (*PickSupplyDropLocationOG)(AFortAthenaMapInfo* MapInfo, FVector* outLocation, __int64 Center, float Radius) = decltype(PickSupplyDropLocationOG)(__int64(GetModuleHandleA(0)) + 0x14F83A0);
static __int64 (*CantBuild)(UWorld*, UObject*, FVector, FRotator, char, void*, char*) = decltype(CantBuild)(__int64(GetModuleHandleW(0)) + 0x19EE0E0);
static ABuildingSMActor* (*ReplaceBuildingActor)(ABuildingSMActor* BuildingSMActor, unsigned int a2, UObject* a3, unsigned int a4, int a5, bool bMirrored, AFortPlayerControllerAthena* PC) = decltype(ReplaceBuildingActor)(__int64(GetModuleHandleW(0)) + 0x1782040);
static void(*RemoveFromAlivePlayers)(AFortGameModeAthena*, AFortPlayerControllerAthena*, APlayerState*, AFortPlayerPawn*, UFortWeaponItemDefinition*, EDeathCause, char) = decltype(RemoveFromAlivePlayers)(__int64(GetModuleHandleW(0)) + 0x15413F0);
static void (*OnRep_ZiplineState)(AFortPlayerPawn* Pawn) = decltype(OnRep_ZiplineState)(__int64(GetModuleHandleW(0)) + 0x1DC5980);

TArray<APlayerController*> GivenLootPlayers;
TArray<FVector> PickedllamaLocations;

void Log(string msg)
{
	cout << "[MGS]: " << msg << endl;
}

void HookVTable(void* Base, int Idx, void* Detour, void** OG)
{
	DWORD oldProtection;

	void** VTable = *(void***)Base;

	if (OG)
	{
		*OG = VTable[Idx];
	}

	VirtualProtect(&VTable[Idx], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtection);

	VTable[Idx] = Detour;

	VirtualProtect(&VTable[Idx], sizeof(void*), oldProtection, NULL);
}

inline FQuat RotatorToQuat(FRotator Rotation) 
{ 
	FQuat Quat;
	const float DEG_TO_RAD = 3.14159f / 180.0f;
	const float DIVIDE_BY_2 = DEG_TO_RAD / 2.0f;

	float SP = sin(Rotation.Pitch * DIVIDE_BY_2);
	float CP = cos(Rotation.Pitch * DIVIDE_BY_2);
	float SY = sin(Rotation.Yaw * DIVIDE_BY_2);
	float CY = cos(Rotation.Yaw * DIVIDE_BY_2);
	float SR = sin(Rotation.Roll * DIVIDE_BY_2);
	float CR = cos(Rotation.Roll * DIVIDE_BY_2);

	Quat.X = CR * SP * SY - SR * CP * CY;
	Quat.Y = -CR * SP * CY - SR * CP * SY;
	Quat.Z = CR * CP * SY - SR * SP * CY;
	Quat.W = CR * CP * CY + SR * SP * SY;

	return Quat;
}

template<typename T>
static inline T* SpawnAActor(FVector Loc = { 0,0,0 }, FRotator Rot = { 0,0,0 }, AActor* Owner = nullptr)
{
	FTransform Transform{};
	Transform.Scale3D = { 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = RotatorToQuat(Rot);

	AActor* NewActor = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), T::StaticClass(), Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner);
	return (T*)UGameplayStatics::FinishSpawningActor(NewActor, Transform);
}

template<typename T>
inline T* SpawnActor(FVector Loc, FRotator Rot = FRotator(), AActor* Owner = nullptr, SDK::UClass* Class = T::StaticClass(), ESpawnActorCollisionHandlingMethod Handle = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
{
	FTransform Transform{};
	Transform.Scale3D = FVector{ 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = RotatorToQuat(Rot);

	return (T*)UGameplayStatics::FinishSpawningActor(UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, Handle, Owner), Transform);
}

template<typename T>
T* Actors(UClass* Class = T::StaticClass(), FVector Loc = {}, FRotator Rot = {}, AActor* Owner = nullptr)
{
	return SpawnActor<T>(Loc, Rot, Owner, Class);
}

template<typename T>
T* GetDefaultObject()
{
	return (T*)T::StaticClass()->DefaultObject;
}

AFortPickupAthena* SpawnPickup(UFortItemDefinition* ItemDef, int OverrideCount, int LoadedAmmo, FVector Loc, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source, AFortPawn* Pawn = nullptr)
{
	auto SpawnedPickup = Actors<AFortPickupAthena>(AFortPickupAthena::StaticClass(), Loc);
	SpawnedPickup->bRandomRotation = true;

	auto& PickupEntry = SpawnedPickup->PrimaryPickupItemEntry;
	PickupEntry.ItemDefinition = ItemDef;
	PickupEntry.Count = OverrideCount;
	PickupEntry.LoadedAmmo = LoadedAmmo;
	PickupEntry.ReplicationKey++;
	SpawnedPickup->OnRep_PrimaryPickupItemEntry();
	SpawnedPickup->PawnWhoDroppedPickup = Pawn;

	SpawnedPickup->TossPickup(Loc, Pawn, -1, true, false, SourceType, Source);

	SpawnedPickup->SetReplicateMovement(true);
	SpawnedPickup->MovementComponent = (UProjectileMovementComponent*)GetDefaultObject<UGameplayStatics>()->SpawnObject(UProjectileMovementComponent::StaticClass(), SpawnedPickup);

	if (SourceType == EFortPickupSourceTypeFlag::Container)
	{
		SpawnedPickup->bTossedFromContainer = true;
		SpawnedPickup->OnRep_TossedFromContainer();
	}

	return SpawnedPickup;
}

map<AFortPickup*, float> PickupLifetimes;
AFortPickupAthena* SpawnStack(APlayerPawn_Athena_C* Pawn, UFortItemDefinition* Def, int Count, bool giveammo = false, int ammo = 0)
{
	auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

	FVector Loc = Pawn->K2_GetActorLocation();
	AFortPickupAthena* Pickup = Actors<AFortPickupAthena>(AFortPickupAthena::StaticClass(), Loc);
	Pickup->bReplicates = true;
	PickupLifetimes[Pickup] = Statics->GetTimeSeconds(UWorld::GetWorld());
	Pickup->PawnWhoDroppedPickup = Pawn;
	Pickup->PrimaryPickupItemEntry.Count = Count;
	Pickup->PrimaryPickupItemEntry.ItemDefinition = Def;
	if (giveammo)
	{
		Pickup->PrimaryPickupItemEntry.LoadedAmmo = ammo;
	}
	Pickup->PrimaryPickupItemEntry.ReplicationKey++;

	Pickup->OnRep_PrimaryPickupItemEntry();
	Pickup->TossPickup(Loc, Pawn, 6, true, true, EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::Unset);

	Pickup->MovementComponent = (UProjectileMovementComponent*)Statics->SpawnObject(UProjectileMovementComponent::StaticClass(), Pickup);
	Pickup->MovementComponent->bReplicates = true;
	((UProjectileMovementComponent*)Pickup->MovementComponent)->SetComponentTickEnabled(true);

	return Pickup;
}

static AFortPickupAthena* SpawnPickup(FFortItemEntry ItemEntry, FVector Location, EFortPickupSourceTypeFlag PickupSource = EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource SpawnSource = EFortPickupSpawnSource::Unset)
{
	auto Pickup = SpawnPickup(ItemEntry.ItemDefinition, ItemEntry.Count, ItemEntry.LoadedAmmo, Location, PickupSource, SpawnSource);
	return Pickup;
}

FVector PickSupplyDropLocation(AFortAthenaMapInfo* MapInfo, FVector Center, float Radius)
{
	if (!PickSupplyDropLocationOG)//if this happens then ggs
		return FVector(0, 0, 0);

	const float MinDistance = 10000.0f; //idk whats the best distance

	for (int i = 0; i < 20; i++)
	{
		FVector loc = FVector(0, 0, 0);
		PickSupplyDropLocationOG(MapInfo, &loc, (__int64)&Center, Radius);

		bool bTooClose = false;
		for (const auto& other : PickedllamaLocations)
		{
			float dx = loc.X - other.X;
			float dy = loc.Y - other.Y;
			float dz = loc.Z - other.Z;

			float distSquared = dx * dx + dy * dy + dz * dz;

			if (distSquared < MinDistance * MinDistance)
			{
				bTooClose = true;
				break;
			}
		}

		if (!bTooClose)
		{
			PickedllamaLocations.Add(loc);
			return loc;
		}
	}

	return FVector(0, 0, 0);
}

template <typename T>
static inline T* StaticFindObject(std::wstring ObjectName)
{
	if constexpr (std::is_same_v<T, UObject>)
	{
		return (T*)StaticFindObjectOG(nullptr, nullptr, ObjectName.c_str(), false);
	}
	else
	{
		return (T*)StaticFindObjectOG(T::StaticClass(), nullptr, ObjectName.c_str(), false);
	}
}

template<typename T = UObject>
static inline T* StaticLoadObject(const std::string& Name)
{
	auto ConvName = std::wstring(Name.begin(), Name.end());

	T* Object = StaticFindObject<T>(ConvName);

	if (!Object)
	{
		Object = (T*)StaticLoadObjectOG(T::StaticClass(), nullptr, ConvName.c_str(), nullptr, 0, nullptr, false, nullptr);
	}

	return Object;
}

template<typename T>
inline T* Cast(UObject* Object)
{
	if (!Object || !Object->IsA(T::StaticClass()))
		return nullptr;
	return (T*)Object;
}

template<typename T>
inline vector<T*> GetAllObjectsOfClass(UClass* Class = T::StaticClass())
{
	std::vector<T*> Objects{};

	for (int i = 0; i < UObject::GObjects->Num(); ++i)
	{
		UObject* Object = UObject::GObjects->GetByIndex(i);

		if (!Object)
			continue;

		if (Object->GetFullName().contains("Default"))
			continue;

		if (Object->GetFullName().contains("Test"))
			continue;

		if (Object->IsA(Class) && !Object->IsDefaultObject())
		{
			Objects.push_back((T*)Object);
		}
	}

	return Objects;
}

template <typename _It>
static _It* GetInterface(UObject* Object)
{
	return ((_It * (*)(UObject*, UClass*)) (InSDKUtils::GetImageBase() + 0x7A4D910))(Object, _It::StaticClass());
}

void LateGameAircraftThread(FVector BattleBusLocation)
{
	auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

	Sleep(70);

	GameState->bAircraftIsLocked = false;

	GameMode->OnAircraftExitedDropZone(GameState->GetAircraft(0));

	GameState->GamePhase = EAthenaGamePhase::SafeZones;

	GameState->SafeZonesStartTime = 1;

}

static void StartAircraft()
{

	auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

	auto GameMode = Cast<AFortGameModeAthena>(UWorld::GetWorld()->AuthorityGameMode);
	auto GameState = Cast<AFortGameStateAthena>(GameMode->GameState);

	float Duration = 11.f;
	float EarlyDuration = 11.f;

	auto TimeSeconds = Statics->GetTimeSeconds(UWorld::GetWorld());

	GameState->WarmupCountdownEndTime = TimeSeconds + Duration;
	GameMode->WarmupCountdownDuration = Duration;

	GameState->WarmupCountdownStartTime = TimeSeconds;
	GameMode->WarmupEarlyCountdownDuration = EarlyDuration;
}

template<typename T>
static inline T* SpawnActorClass(FVector Loc = { 0,0,0 }, FRotator Rot = { 0,0,0 }, UClass* Class = nullptr, AActor* Owner = nullptr)
{
	FTransform Transform{};
	Transform.Scale3D = { 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = RotatorToQuat(Rot);

	AActor* NewActor = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner);
	return (T*)UGameplayStatics::FinishSpawningActor(NewActor, Transform);
}