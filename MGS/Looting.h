#pragma once
#include"Utils.h"
#include"Inventory.h"
#include"GUI.h"
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <string>
#include <cstdlib>
#include <ctime>   


static FFortLootTierData* GetLootTierData(std::vector<FFortLootTierData*>& LootTierData)
{
    float TotalWeight = 0;

    for (auto Item : LootTierData)
        TotalWeight += Item->Weight;

    float RandomNumber = UKismetMathLibrary::RandomFloatInRange(0, TotalWeight);

    FFortLootTierData* SelectedItem = nullptr;


    for (auto Item : LootTierData)
    {

        if (RandomNumber <= Item->Weight)
        {
            SelectedItem = Item;
            break;
        }

        RandomNumber -= Item->Weight;
    }

    if (!SelectedItem)
        return GetLootTierData(LootTierData);

    return SelectedItem;
}

static FFortLootPackageData* GetLootPackage(std::vector<FFortLootPackageData*>& LootPackages)
{
    float TotalWeight = 0;

    for (auto Item : LootPackages)
        TotalWeight += Item->Weight;

    float RandomNumber = UKismetMathLibrary::RandomFloatInRange(0, TotalWeight);

    FFortLootPackageData* SelectedItem = nullptr;

    for (auto Item : LootPackages)
    {
        if (RandomNumber <= Item->Weight)
        {
            SelectedItem = Item;
            break;
        }

        RandomNumber -= Item->Weight;
    }

    if (!SelectedItem)
        return GetLootPackage(LootPackages);

    return SelectedItem;
}

int GetClipSize(UFortItemDefinition* ItemDef) {
    if (auto RangedDef = Cast<UFortWeaponRangedItemDefinition>(ItemDef)) {
        auto DataTable = RangedDef->WeaponStatHandle.DataTable;
        auto RowName = RangedDef->WeaponStatHandle.RowName;

        if (DataTable && RowName.ComparisonIndex) {
            auto& RowMap = *(TMap<FName, FFortRangedWeaponStats*>*)(__int64(DataTable) + 0x30);

            for (auto& Pair : RowMap) {
                FName CurrentRowName = Pair.Key();
                FFortRangedWeaponStats* PackageData = Pair.Value();

                if (CurrentRowName == RowName && PackageData) {
                    return PackageData->ClipSize;
                }
            }
        }
    }

    return 0;
}

std::vector<FFortItemEntry> PickLootDrops(FName TierGroupName, int recursive = 0)
{

    std::vector<FFortItemEntry> LootDrops;

    if (recursive >= 5)
    {
        return LootDrops;
    }

    auto TierGroupFName = TierGroupName;

    static std::vector<UDataTable*> LTDTables;
    static std::vector<UDataTable*> LPTables;

    static bool First = false;

    if (!First)
    {
        First = true;

        auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

        UDataTable* MainLTD = StaticLoadObject<UDataTable>(UKismetStringLibrary::Conv_NameToString(GameState->CurrentPlaylistInfo.BasePlaylist->LootTierData.ObjectID.AssetPathName).ToString());;
        UDataTable* MainLP = StaticLoadObject<UDataTable>(UKismetStringLibrary::Conv_NameToString(GameState->CurrentPlaylistInfo.BasePlaylist->LootPackages.ObjectID.AssetPathName).ToString());

        if (!MainLTD)
            MainLTD = UObject::FindObject<UDataTable>("AthenaLootTierData_Client");

        if (!MainLP)
            MainLP = UObject::FindObject<UDataTable>("AthenaLootPackages_Client");

        LTDTables.push_back(MainLTD);

        LPTables.push_back(MainLP);
    }

    std::vector<FFortLootTierData*> TierGroupLTDs;

    for (int p = 0; p < LTDTables.size(); p++)
    {
        auto LTD = LTDTables[p];
        auto& LTDRowMap = LTD->RowMap;

        auto LTDRowMapNum = LTDRowMap.Elements.Num();


        for (int i = 0; i < LTDRowMapNum; i++)
        {
            auto& CurrentLTD = LTDRowMap.Elements[i];
            auto TierData = (FFortLootTierData*)CurrentLTD.Value();


            if (TierGroupName == TierData->TierGroup && TierData->Weight != 0)
            {
                TierGroupLTDs.push_back(TierData);
            }
        }
    }

    FFortLootTierData* ChosenRowLootTierData = GetLootTierData(TierGroupLTDs);

    if (ChosenRowLootTierData->NumLootPackageDrops <= 0)
        return PickLootDrops(TierGroupName, ++recursive);

    if (ChosenRowLootTierData->LootPackageCategoryMinArray.Num() != ChosenRowLootTierData->LootPackageCategoryWeightArray.Num() ||
        ChosenRowLootTierData->LootPackageCategoryMinArray.Num() != ChosenRowLootTierData->LootPackageCategoryMaxArray.Num())
        return PickLootDrops(TierGroupName, ++recursive);

    int MinimumLootDrops = 0;
    float NumLootPackageDrops = std::floor(ChosenRowLootTierData->NumLootPackageDrops);

    if (ChosenRowLootTierData->LootPackageCategoryMinArray.Num())
    {
        for (int i = 0; i < ChosenRowLootTierData->LootPackageCategoryMinArray.Num(); i++)
        {
            if (ChosenRowLootTierData->LootPackageCategoryMinArray[i] > 0)
            {
                MinimumLootDrops += ChosenRowLootTierData->LootPackageCategoryMinArray[i];
            }
        }
    }

    int SumLootPackageCategoryWeightArray = 0;

    for (int i = 0; i < ChosenRowLootTierData->LootPackageCategoryWeightArray.Num(); i++)
    {
        auto CategoryWeight = ChosenRowLootTierData->LootPackageCategoryWeightArray[i];

        if (CategoryWeight > 0)
        {
            auto CategoryMaxArray = ChosenRowLootTierData->LootPackageCategoryMaxArray[i];

            if (CategoryMaxArray < 0)
            {
                SumLootPackageCategoryWeightArray += CategoryWeight;
            }
        }
    }

    int SumLootPackageCategoryMinArray = 0;

    for (int i = 0; i < ChosenRowLootTierData->LootPackageCategoryMinArray.Num(); i++)
    {
        auto CategoryWeight = ChosenRowLootTierData->LootPackageCategoryMinArray[i];

        if (CategoryWeight > 0)
        {
            auto CategoryMaxArray = ChosenRowLootTierData->LootPackageCategoryMaxArray[i];

            if (CategoryMaxArray < 0)
            {
                SumLootPackageCategoryMinArray += CategoryWeight;
            }
        }
    }

    if (SumLootPackageCategoryWeightArray > SumLootPackageCategoryMinArray)
        return PickLootDrops(TierGroupName, ++recursive);

    std::vector<FFortLootPackageData*> TierGroupLPs;

    for (int p = 0; p < LPTables.size(); p++)
    {
        auto LP = LPTables[p];
        auto& LPRowMap = LP->RowMap;

        for (int i = 0; i < LPRowMap.Elements.Num(); i++)
        {
            auto& CurrentLP = LPRowMap.Elements[i];
            auto LootPackage = (FFortLootPackageData*)CurrentLP.Value();

            if (!LootPackage)
                continue;

            if (LootPackage->LootPackageID == ChosenRowLootTierData->LootPackage && LootPackage->Weight != 0)
            {
                TierGroupLPs.push_back(LootPackage);
            }
        }
    }

    auto ChosenLootPackageName = ChosenRowLootTierData->LootPackage.ToString();


    bool bIsWorldList = ChosenLootPackageName.contains("WorldList");


    LootDrops.reserve(NumLootPackageDrops);

    for (float i = 0; i < NumLootPackageDrops; i++)
    {
        if (i >= TierGroupLPs.size())
            break;

        auto TierGroupLP = TierGroupLPs.at(i);
        auto TierGroupLPStr = TierGroupLP->LootPackageCall.ToString();

        if (TierGroupLPStr.contains(".Empty"))
        {
            NumLootPackageDrops++;
            continue;
        }

        std::vector<FFortLootPackageData*> lootPackageCalls;

        if (bIsWorldList)
        {
            for (int j = 0; j < TierGroupLPs.size(); j++)
            {
                auto& CurrentLP = TierGroupLPs.at(j);

                if (CurrentLP->Weight != 0)
                    lootPackageCalls.push_back(CurrentLP);
            }
        }
        else
        {
            for (int p = 0; p < LPTables.size(); p++)
            {
                auto LPRowMap = LPTables[p]->RowMap;

                for (int j = 0; j < LPRowMap.Elements.Num(); j++)
                {
                    auto& CurrentLP = LPRowMap.Elements[j];

                    auto LootPackage = (FFortLootPackageData*)CurrentLP.Value();

                    if (LootPackage->LootPackageID.ToString() == TierGroupLPStr && LootPackage->Weight != 0)
                    {
                        lootPackageCalls.push_back(LootPackage);
                    }
                }
            }
        }

        if (lootPackageCalls.size() == 0)
        {
            NumLootPackageDrops++;
            continue;
        }


        FFortLootPackageData* LootPackageCall = GetLootPackage(lootPackageCalls);

        if (!LootPackageCall)
            continue;

        auto ItemDef = LootPackageCall->ItemDefinition.Get();

        if (!ItemDef)
        {
            NumLootPackageDrops++;
            continue;
        }

        FFortItemEntry LootDropEntry{};

        LootDropEntry.ItemDefinition = ItemDef;
        LootDropEntry.LoadedAmmo = GetClipSize(Cast<UFortWeaponItemDefinition>(ItemDef));
        LootDropEntry.Count = LootPackageCall->Count;

        LootDrops.push_back(LootDropEntry);
    }

    return LootDrops;
}

char SpawnLoot(ABuildingContainer* BuildingContainer)
{
    string ClassName = BuildingContainer->Class->GetName();

    auto SearchLootTierGroup = BuildingContainer->SearchLootTierGroup;
    EFortPickupSpawnSource SpawnSource = EFortPickupSpawnSource::Unset;

    EFortPickupSourceTypeFlag PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Container;

    static auto Loot_Treasure = UKismetStringLibrary::Conv_StringToName(L"Loot_Treasure");
    static auto Loot_Ammo = UKismetStringLibrary::Conv_StringToName(L"Loot_Ammo");
    static auto Loot_AthenaFloorLoot = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaFloorLoot");
    static auto Loot_AthenaFloorLoot_Warmup = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaFloorLoot_Warmup");

    if (SearchLootTierGroup == Loot_AthenaFloorLoot || SearchLootTierGroup == Loot_AthenaFloorLoot_Warmup)
    {
        PickupSourceTypeFlags = EFortPickupSourceTypeFlag::FloorLoot;
    }

    if (!LateGame)
    {
        BuildingContainer->bAlreadySearched = true;
        BuildingContainer->SearchBounceData.SearchAnimationCount++;
        BuildingContainer->OnRep_bAlreadySearched();
    }
    else if (LateGame && SearchLootTierGroup == Loot_AthenaFloorLoot_Warmup)
    {
        BuildingContainer->bAlreadySearched = true;
        BuildingContainer->SearchBounceData.SearchAnimationCount++;
        BuildingContainer->OnRep_bAlreadySearched();

        auto LootDrops = PickLootDrops(SearchLootTierGroup);

        auto CorrectLocation = BuildingContainer->K2_GetActorLocation() + (BuildingContainer->GetActorForwardVector() * BuildingContainer->LootSpawnLocation_Athena.X) + (BuildingContainer->GetActorRightVector() * BuildingContainer->LootSpawnLocation_Athena.Y) + (BuildingContainer->GetActorUpVector() * BuildingContainer->LootSpawnLocation_Athena.Z);

        for (auto& LootDrop : LootDrops)
        {
            if (LootDrop.Count > 0)
            {
                SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, PickupSourceTypeFlags, SpawnSource);

                if (SearchLootTierGroup == Loot_AthenaFloorLoot || SearchLootTierGroup == Loot_AthenaFloorLoot_Warmup)
                {
                    if (LootDrop.ItemDefinition->GetName() == "WID_Athena_HappyGhost")
                    {
                        return 0;
                    }

                    UFortAmmoItemDefinition* AmmoDef = (UFortAmmoItemDefinition*)((UFortWeaponRangedItemDefinition*)LootDrop.ItemDefinition)->GetAmmoWorldItemDefinition_BP();

                    if (AmmoDef && LootDrop.ItemDefinition != AmmoDef && AmmoDef->DropCount > 0)
                    {
                        SpawnPickup(AmmoDef, AmmoDef->DropCount, 0, CorrectLocation, PickupSourceTypeFlags, SpawnSource);
                    }
                }
            }

        }
    }

    if (!LateGame)
    {

        if (SearchLootTierGroup == Loot_Treasure)
        {
            EFortPickupSourceTypeFlag PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Container;

            SearchLootTierGroup = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaTreasure");
            SpawnSource = EFortPickupSpawnSource::Chest;
        }

        if (SearchLootTierGroup == Loot_Ammo)
        {
            EFortPickupSourceTypeFlag PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Container;

            SearchLootTierGroup = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaAmmoLarge");
            SpawnSource = EFortPickupSpawnSource::AmmoBox;
        }

        if (ClassName.contains("Tiered_Chest_Athena_FactionChest"))
        {
            for (int i = 0; i < 2; i++)
            {
                auto LootDrops = PickLootDrops(SearchLootTierGroup);

                auto CorrectLocation = BuildingContainer->K2_GetActorLocation() + (BuildingContainer->GetActorForwardVector() * BuildingContainer->LootSpawnLocation_Athena.X) + (BuildingContainer->GetActorRightVector() * BuildingContainer->LootSpawnLocation_Athena.Y) + (BuildingContainer->GetActorUpVector() * BuildingContainer->LootSpawnLocation_Athena.Z);

                for (auto& LootDrop : LootDrops)
                {
                    SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, PickupSourceTypeFlags, SpawnSource);

                    UFortAmmoItemDefinition* AmmoDef = (UFortAmmoItemDefinition*)((UFortWeaponRangedItemDefinition*)LootDrop.ItemDefinition)->GetAmmoWorldItemDefinition_BP();

                    if (AmmoDef && LootDrop.ItemDefinition != AmmoDef && AmmoDef->DropCount > 0)
                    {
                        SpawnPickup(AmmoDef, AmmoDef->DropCount, 0, CorrectLocation, PickupSourceTypeFlags, SpawnSource);
                    }
                }
            }
            return true;
        }

        else if (ClassName.contains("Tiered_Chest"))
        {
            auto LootDrops = PickLootDrops(SearchLootTierGroup);

            auto CorrectLocation = BuildingContainer->K2_GetActorLocation() + (BuildingContainer->GetActorForwardVector() * BuildingContainer->LootSpawnLocation_Athena.X) + (BuildingContainer->GetActorRightVector() * BuildingContainer->LootSpawnLocation_Athena.Y) + (BuildingContainer->GetActorUpVector() * BuildingContainer->LootSpawnLocation_Athena.Z);

            for (auto& LootDrop : LootDrops)
            {
                SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, PickupSourceTypeFlags, SpawnSource);
            }

            static auto Wood = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/WoodItemData.WoodItemData");
            static auto Metal = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/MetalItemData.MetalItemData");
            static auto Stone = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/StoneItemData.StoneItemData");

            UFortItemDefinition* Mats = (rand() % 40 > 20) ? ((rand() % 20 > 10) ? Wood : Stone) : Metal;

            SpawnPickup(Mats, 30, 0, CorrectLocation, PickupSourceTypeFlags, SpawnSource);

            return true;
        }

        else
        {
            auto LootDrops = PickLootDrops(SearchLootTierGroup);

            auto CorrectLocation = BuildingContainer->K2_GetActorLocation() + (BuildingContainer->GetActorForwardVector() * BuildingContainer->LootSpawnLocation_Athena.X) + (BuildingContainer->GetActorRightVector() * BuildingContainer->LootSpawnLocation_Athena.Y) + (BuildingContainer->GetActorUpVector() * BuildingContainer->LootSpawnLocation_Athena.Z);

            for (auto& LootDrop : LootDrops)
            {
                if (LootDrop.Count > 0)
                {
                    SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, PickupSourceTypeFlags, SpawnSource);

                    if (SearchLootTierGroup == Loot_AthenaFloorLoot || SearchLootTierGroup == Loot_AthenaFloorLoot_Warmup)
                    {
                        if (LootDrop.ItemDefinition->GetName() == "WID_Athena_HappyGhost")
                        {
                            return 0;
                        }

                        UFortAmmoItemDefinition* AmmoDef = (UFortAmmoItemDefinition*)((UFortWeaponRangedItemDefinition*)LootDrop.ItemDefinition)->GetAmmoWorldItemDefinition_BP();

                        if (AmmoDef && LootDrop.ItemDefinition != AmmoDef && AmmoDef->DropCount > 0)
                        {
                            SpawnPickup(AmmoDef, AmmoDef->DropCount, 0, CorrectLocation, PickupSourceTypeFlags, SpawnSource);
                        }
                    }
                }

            }
        }
    }

    return true;
}

void SpawnFloorLoot()
{
    auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

    TArray<AActor*> FloorLootSpawners;
    UClass* SpawnerClass = StaticLoadObject<UClass>("/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_Warmup.Tiered_Athena_FloorLoot_Warmup_C");
    Statics->GetAllActorsOfClass(UWorld::GetWorld(), SpawnerClass, &FloorLootSpawners);

    for (size_t i = 0; i < FloorLootSpawners.Num(); i++)
    {
        FloorLootSpawners[i]->K2_DestroyActor();
    }

    FloorLootSpawners.Free();

    if (!LateGame)
    {
        SpawnerClass = StaticLoadObject<UClass>("/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_01.Tiered_Athena_FloorLoot_01_C");
        Statics->GetAllActorsOfClass(UWorld::GetWorld(), SpawnerClass, &FloorLootSpawners);

        for (size_t i = 0; i < FloorLootSpawners.Num(); i++)
        {
            FloorLootSpawners[i]->K2_DestroyActor();
        }

        FloorLootSpawners.Free();
    }
}

void SpawnLlamas()
{
    int LlamasSpawned = 0;
    auto LlamasToSpawn = (rand() % 3) + 3;
    Log(string("Spawned ") + to_string(LlamasToSpawn) + " Llamas");

    auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

    auto MapInfo = GameState->MapInfo;

    for (int i = 0; i < LlamasToSpawn; i++)
    {
        int Radius = 100000;
        FVector Location = PickSupplyDropLocation(MapInfo, FVector(1, 1, 10000), (float)Radius);

        FRotator RandomYawRotator{};
        RandomYawRotator.Yaw = (float)rand() * 0.010986663f;

        FTransform InitialSpawnTransform{};
        InitialSpawnTransform.Translation = Location;
        InitialSpawnTransform.Rotation = RotatorToQuat(RandomYawRotator);
        InitialSpawnTransform.Scale3D = FVector(1, 1, 1);

        auto LlamaStart = SpawnActor<AFortAthenaSupplyDrop>(Location, RandomYawRotator, nullptr, MapInfo->LlamaClass.Get());

        if (!LlamaStart)
            continue;

        auto GroundLocation = LlamaStart->FindGroundLocationAt(InitialSpawnTransform.Translation);

        LlamaStart->K2_DestroyActor();

        auto Llama = SpawnActor<AFortAthenaSupplyDrop>(GroundLocation, RandomYawRotator, nullptr, MapInfo->LlamaClass.Get());

        Llama->bCanBeDamaged = false;

        if (!Llama)
            continue;
        LlamasSpawned++;
    }
}

//Lategame Loot
std::vector<std::string> Assault_rifle = {
    "/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_VR_Ore_T03.WID_Assault_AutoHigh_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_SR_Ore_T03.WID_Assault_AutoHigh_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_SemiAuto_Athena_VR_Ore_T03.WID_Assault_SemiAuto_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_Heavy_Athena_VR_Ore_T03.WID_Assault_Heavy_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_Heavy_Athena_SR_Ore_T03.WID_Assault_Heavy_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_Heavy_Athena_R_Ore_T03.WID_Assault_Heavy_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_Surgical_Athena_R_Ore_T03.WID_Assault_Surgical_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_Suppressed_Athena_VR_Ore_T03.WID_Assault_Suppressed_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_Suppressed_Athena_SR_Ore_T03.WID_Assault_Suppressed_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_PistolCaliber_AR_Athena_VR_Ore_T03.WID_Assault_PistolCaliber_AR_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_PistolCaliber_AR_Athena_SR_Ore_T03.WID_Assault_PistolCaliber_AR_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_PistolCaliber_AR_Athena_R_Ore_T03.WID_Assault_PistolCaliber_AR_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_Suppressed_Athena_R_Ore_T03.WID_Assault_Suppressed_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_Surgical_Athena_VR_Ore_T03.WID_Assault_Surgical_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_Surgical_Athena_SR_Ore_T03.WID_Assault_Surgical_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_Surgical_Thermal_Athena_VR_Ore_T03.WID_Assault_Surgical_Thermal_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_Surgical_Thermal_Athena_SR_Ore_T03.WID_Assault_Surgical_Thermal_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_SemiAuto_Athena_SR_Ore_T03.WID_Assault_SemiAuto_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_SemiAuto_Athena_R_Ore_T03.WID_Assault_SemiAuto_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_Auto_Athena_R_Ore_T03.WID_Assault_Auto_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_LMGSAW_Athena_VR_Ore_T03.WID_Assault_LMGSAW_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_LMGSAW_Athena_R_Ore_T03.WID_Assault_LMGSAW_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_Infantry_Athena_VR.WID_Assault_Infantry_Athena_VR",
    "/Game/Athena/Items/Weapons/WID_Assault_Infantry_Athena_R.WID_Assault_Infantry_Athena_R",
    "/Game/Athena/Items/Weapons/WID_Assault_Infantry_Athena_SR.WID_Assault_Infantry_Athena_SR"
};

std::vector<std::string> Shotgun = {
    "/Game/Athena/Items/Weapons/WID_Shotgun_BreakBarrel_Athena_VR_Ore_T03.WID_Shotgun_BreakBarrel_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Shotgun_Combat_Athena_VR_Ore_T03.WID_Shotgun_Combat_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Shotgun_Combat_Athena_SR_Ore_T03.WID_Shotgun_Combat_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Shotgun_Combat_Athena_R_Ore_T03.WID_Shotgun_Combat_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Shotgun_SlugFire_Athena_VR.WID_Shotgun_SlugFire_Athena_VR",
    "/Game/Athena/Items/Weapons/WID_Shotgun_SlugFire_Athena_SR.WID_Shotgun_SlugFire_Athena_SR",
    "/Game/Athena/Items/Weapons/WID_Shotgun_SlugFire_Athena_R.WID_Shotgun_SlugFire_Athena_R",
    "/Game/Athena/Items/Weapons/WID_Shotgun_SemiAuto_Athena_VR_Ore_T03.WID_Shotgun_SemiAuto_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Shotgun_HighSemiAuto_Athena_VR_Ore_T03.WID_Shotgun_HighSemiAuto_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Shotgun_HighSemiAuto_Athena_SR_Ore_T03.WID_Shotgun_HighSemiAuto_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Shotgun_AutoDrum_Athena_R_Ore_T03.WID_Shotgun_AutoDrum_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Shotgun_BreakBarrel_Athena_SR_Ore_T03.WID_Shotgun_BreakBarrel_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_VR_Ore_T03.WID_Shotgun_Standard_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03",
};

std::vector<std::string> Mixed = {
    "/Game/Athena/Items/Weapons/WID_Sniper_Auto_Suppressed_Scope_Athena_VR.WID_Sniper_Auto_Suppressed_Scope_Athena_VR",
    "/Game/Athena/Items/Weapons/WID_Pistol_Donut.WID_Pistol_Donut",
    "/Game/Athena/Items/Weapons/WID_Pistol_BurstFireSMG_Athena_R_Ore_T03.WID_Pistol_BurstFireSMG_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_VR_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_AutoHeavyPDW_Athena_VR_Ore_T03.WID_Pistol_AutoHeavyPDW_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/Seasonal/WID_Sniper_Valentine_Athena_VR_Ore_T03.WID_Sniper_Valentine_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_AutoHeavyPDW_Athena_SR_Ore_T03.WID_Pistol_AutoHeavyPDW_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_DualPistol_Suppresseed_Athena_VR_T01.WID_DualPistol_Suppresseed_Athena_VR_T01",
    "/Game/Athena/Items/Weapons/WID_DualPistol_Suppresseed_Athena_R_T01.WID_DualPistol_Suppresseed_Athena_R_T01",
    "/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_AutoHeavy_Athena_R_Ore_T03.WID_Pistol_AutoHeavy_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Launcher_Rocket_Athena_VR_Ore_T03.WID_Launcher_Rocket_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Launcher_Rocket_Athena_SR_Ore_T03.WID_Launcher_Rocket_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Launcher_Rocket_Athena_R_Ore_T03.WID_Launcher_Rocket_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/Seasonal/WID_Launcher_Pumpkin_Athena_VR_Ore_T03.WID_Launcher_Pumpkin_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/Seasonal/WID_Launcher_Pumpkin_Athena_SR_Ore_T03.WID_Launcher_Pumpkin_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/Seasonal/WID_Launcher_Pumpkin_Athena_R_Ore_T03.WID_Launcher_Pumpkin_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_AutoHeavySuppressed_Athena_R_Ore_T03.WID_Pistol_AutoHeavySuppressed_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_AutoHeavySuppressed_Athena_VR_Ore_T03.WID_Pistol_AutoHeavySuppressed_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_AutoHeavySuppressed_Athena_SR_Ore_T03.WID_Pistol_AutoHeavySuppressed_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_R_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_Flintlock_Athena_C.WID_Pistol_Flintlock_Athena_C",
    "/Game/Athena/Items/Weapons/WID_DualPistol_SemiAuto_Athena_VR_Ore_T03.WID_DualPistol_SemiAuto_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_LMG_Athena_VR_Ore_T03.WID_Assault_LMG_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_LMG_Athena_SR_Ore_T03.WID_Assault_LMG_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_LMG_Athena_R_Ore_T03.WID_Assault_LMG_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_Scavenger_Athena_VR_Ore_T03.WID_Pistol_Scavenger_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_Scavenger_Athena_R_Ore_T03.WID_Pistol_Scavenger_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_Scavenger_Athena_SR_Ore_T03.WID_Pistol_Scavenger_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Sniper_Standard_Scope_Athena_VR_Ore_T03.WID_Sniper_Standard_Scope_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Sniper_Standard_Scope_Athena_VeryRare_Ore_T03.WID_Sniper_Standard_Scope_Athena_VeryRare_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Sniper_Standard_Scope_Athena_SuperRare_Ore_T03.WID_Sniper_Standard_Scope_Athena_SuperRare_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Sniper_Standard_Scope_Athena_SR_Ore_T03.WID_Sniper_Standard_Scope_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_SemiAuto_Athena_VR_Ore_T03.WID_Pistol_SemiAuto_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Sniper_Suppressed_Scope_Athena_VR_Ore_T03.WID_Sniper_Suppressed_Scope_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Sniper_Suppressed_Scope_Athena_SR_Ore_T03.WID_Sniper_Suppressed_Scope_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Sniper_Suppressed_Scope_Athena_R_Ore_T03.WID_Sniper_Suppressed_Scope_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_SemiAuto_Athena_SR_Ore_T03.WID_Pistol_SemiAuto_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Assault_AutoDrum_Athena_R_Ore_T03.WID_Assault_AutoDrum_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_SemiAuto_Suppressed_Athena_SR_Ore_T03.WID_Pistol_SemiAuto_Suppressed_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Launcher_Military_Athena_VR_Ore_T03.WID_Launcher_Military_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Launcher_Military_Athena_SR_Ore_T03.WID_Launcher_Military_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_AutoHeavyPDW_Athena_R_Ore_T03.WID_Pistol_AutoHeavyPDW_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Athena_SMG_VR.WID_Athena_SMG_VR",
    "/Game/Athena/Items/Weapons/WID_Athena_SMG_SR.WID_Athena_SMG_SR",
    "/Game/Athena/Items/Weapons/WID_Launcher_Grenade_Athena_VR_Ore_T03.WID_Launcher_Grenade_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_Revolver_SingleAction_Athena_VR.WID_Pistol_Revolver_SingleAction_Athena_VR",
    "/Game/Athena/Items/Weapons/WID_Pistol_Revolver_SingleAction_Athena_R.WID_Pistol_Revolver_SingleAction_Athena_R",
    "/Game/Athena/Items/Weapons/WID_Launcher_Grenade_Athena_SR_Ore_T03.WID_Launcher_Grenade_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Launcher_Grenade_Athena_R_Ore_T03.WID_Launcher_Grenade_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_DualPistol_SemiAuto_Athena_SR_Ore_T03.WID_DualPistol_SemiAuto_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_C_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_C_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Sniper_Auto_Suppressed_Scope_Athena_SR.WID_Sniper_Auto_Suppressed_Scope_Athena_SR",
    "/Game/Athena/Items/Weapons/WID_Sniper_Auto_Suppressed_Scope_Athena_R.WID_Sniper_Auto_Suppressed_Scope_Athena_R",
    "/Game/Athena/Items/Weapons/LTM/WID_Hook_Gun_Slide.WID_Hook_Gun_Slide",
    "/Game/Athena/Items/Weapons/WID_Sniper_Heavy_Athena_VR_Ore_T03.WID_Sniper_Heavy_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Sniper_Heavy_Athena_SR_Ore_T03.WID_Sniper_Heavy_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Sniper_Heavy_Athena_R_Ore_T03.WID_Sniper_Heavy_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_HandCannon_Athena_VR_Ore_T03.WID_Pistol_HandCannon_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_HandCannon_Athena_SR_Ore_T03.WID_Pistol_HandCannon_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_GrenadeLauncher_Prox_Athena_VR.WID_GrenadeLauncher_Prox_Athena_VR",
    "/Game/Athena/Items/Weapons/WID_Pistol_Scoped_Athena_VR_Ore_T03.WID_Pistol_Scoped_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_Scoped_Athena_SR_Ore_T03.WID_Pistol_Scoped_Athena_SR_Ore_T03"
    "/Game/Athena/Items/Weapons/WID_GrenadeLauncher_Prox_Athena_SR.WID_GrenadeLauncher_Prox_Athena_SR",
    "/Game/Athena/Items/Weapons/WID_Sniper_NoScope_Athena_R_Ore_T03.WID_Sniper_NoScope_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_Standard_Athena_VR.WID_Pistol_Standard_Athena_VR",
    "/Game/Athena/Items/Weapons/WID_Pistol_Standard_Athena_SR.WID_Pistol_Standard_Athena_SR",
    "/Game/Athena/Items/Weapons/WID_Pistol_SemiAuto_Athena_R_Ore_T03.WID_Pistol_SemiAuto_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_RapidFireSMG_Athena_R_Ore_T03.WID_Pistol_RapidFireSMG_Athena_R_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_RapidFireSMG_Athena_VR_Ore_T03.WID_Pistol_RapidFireSMG_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_RapidFireSMG_Athena_SR_Ore_T03.WID_Pistol_RapidFireSMG_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_SixShooter_Athena_VR_Ore_T03.WID_Pistol_SixShooter_Athena_VR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_Sixshooter_Athena_SR_Ore_T03.WID_Pistol_SixShooter_Athena_SR_Ore_T03",
    "/Game/Athena/Items/Weapons/WID_Pistol_SixShooter_Athena_R_Ore_T03.WID_Pistol_SixShooter_Athena_R_Ore_T03",
};

std::vector<std::string> Consumables = {
    "/Game/Athena/Items/Consumables/Shields/Athena_Shields.Athena_Shields",
    "/Game/Athena/Items/Consumables/ShieldSmall/Athena_ShieldSmall.Athena_ShieldSmall",
    "/Game/Athena/Items/Consumables/Balloons/Athena_Balloons_Consumable.Athena_Balloons_Consumable",
    "/Game/Athena/Items/Consumables/Bandage/Athena_Bandage.Athena_Bandage",
    "/Game/Athena/Items/Consumables/DanceGrenade/Athena_DanceGrenade.Athena_DanceGrenade",
    "/Game/Athena/Items/Consumables/Bush/Athena_Bush.Athena_Bush",
    "/Game/Athena/Items/Consumables/ChillBronco/Athena_ChillBronco.Athena_ChillBronco",
    "/Game/Athena/Items/Consumables/StickyGrenade/Athena_StickyGrenade.Athena_StickyGrenade",
    "/Game/Athena/Items/Consumables/ShockwaveGrenade/Athena_ShockGrenade.Athena_ShockGrenade",
    "/Game/Athena/Items/Consumables/TNT/Athena_TNT.Athena_TNT",
    "/Game/Athena/Items/Consumables/Flopper/WID_Athena_Flopper.WID_Athena_Flopper",
    "/Game/Athena/Items/Consumables/DeployableStorm/Athena_DogSweater.Athena_DogSweater",
    "/Game/Athena/Items/Consumables/GasGrenade/Athena_GasGrenade.Athena_GasGrenade",
    "/Game/Athena/Items/Consumables/Grenade/Athena_Grenade.Athena_Grenade",
    "/Game/Athena/Items/Consumables/PurpleStuff/Athena_PurpleStuff.Athena_PurpleStuff",
    "/Game/Athena/Items/Consumables/KnockGrenade/Athena_KnockGrenade.Athena_KnockGrenade",
    "/Game/Athena/Items/Consumables/Medkit/Athena_Medkit.Athena_Medkit",
    "/Game/Athena/Items/Consumables/TowerGrenade/Athena_TowerGrenade.Athena_TowerGrenade",
    "/Game/Athena/Items/Consumables/SmokeGrenade/Athena_SmokeGrenade.Athena_SmokeGrenade",
    "/Game/Athena/Items/Consumables/SuperTowerGrenade/Levels/PortAFort_A/Athena_SuperTowerGrenade_A.Athena_SuperTowerGrenade_A",
    "/Game/Athena/Items/Consumables/C4/Athena_C4.Athena_C4",
    "/Game/Athena/Items/Consumables/RiftItem/Athena_Rift_Item.Athena_Rift_Item",
    "/Game/Athena/Items/Consumables/Flopper/Effective/WID_Athena_Flopper_Effective.WID_Athena_Flopper_Effective",
    "/Game/Athena/Items/Consumables/SilverBlazer/Athena_SilverBlazer_V2.Athena_SilverBlazer_V2",
};

std::vector<std::string> Traps = {
    "/Game/Athena/Items/Traps/TID_Context_BouncePad_Athena.TID_Context_BouncePad_Athena",
    "/Game/Athena/Items/Traps/TID_ContextTrap_Athena.TID_ContextTrap_Athena",
    "/Game/Items/Traps/Floor/TID_Floor_Player_Launch_Pad_Athena.TID_Floor_Player_Launch_Pad_Athena",
};

std::string GetRandomWeapon(const std::vector<std::string>& list) {
    if (list.empty()) return "";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, list.size() - 1);
    return list[dist(gen)];
}

UFortItemDefinition* LoadWeapon(const std::vector<std::string>& Pool)// fixes issue where deff is somehow null
{
    for (int i = 0; i < 2; ++i)
    {
        std::string WeaponPath = GetRandomWeapon(Pool);
        UFortItemDefinition* Def = StaticLoadObject<UFortItemDefinition>(WeaponPath);
        if (Def)
            return Def;
    }
    return nullptr;
}

void __fastcall GiveLoadout(AFortPlayerController* PC)
{
    if (UFortItemDefinition* AssaultRifleDef = LoadWeapon(Assault_rifle))
    {
        GiveItem(PC, AssaultRifleDef, 1, GetClipSize(AssaultRifleDef));

        if (auto* RangedDef = (UFortWeaponRangedItemDefinition*)AssaultRifleDef)
        {
            UFortWorldItemDefinition* AmmoDef = RangedDef->GetAmmoWorldItemDefinition_BP();
            if (AmmoDef)
                GiveItem(PC, AmmoDef, 200, 0);
        }
    }

    if (UFortItemDefinition* ShotGunDef = LoadWeapon(Shotgun))
    {
        GiveItem(PC, ShotGunDef, 1, GetClipSize(ShotGunDef));

        if (auto* RangedDef = (UFortWeaponRangedItemDefinition*)ShotGunDef)
        {
            UFortWorldItemDefinition* AmmoDef = RangedDef->GetAmmoWorldItemDefinition_BP();
            if (AmmoDef)
                GiveItem(PC, AmmoDef, 120, 0);
        }
    }

    if (UFortItemDefinition* RandomDef = LoadWeapon(Mixed))
    {
        GiveItem(PC, RandomDef, 1, GetClipSize(RandomDef));

        if (auto* RangedDef = (UFortWeaponRangedItemDefinition*)RandomDef)
        {
            UFortWorldItemDefinition* AmmoDef = RangedDef->GetAmmoWorldItemDefinition_BP();
            if (AmmoDef)
                GiveItem(PC, AmmoDef, 120, 0);
        }
    }

    if (auto Consumable1Def = LoadWeapon(Consumables))
        GiveItemStack(PC, Consumable1Def, 3, 0);

    if (auto Consumable2Def = LoadWeapon(Consumables))
        GiveItemStack(PC, Consumable2Def, 3, 0);

    if (auto TrapDef = LoadWeapon(Traps))
        GiveItem(PC, TrapDef, 3, 0);

    static UFortItemDefinition* WoodDef = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/WoodItemData.WoodItemData");
    GiveItem(PC, WoodDef, 500, 0);

    static UFortItemDefinition* StoneDef = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/StoneItemData.StoneItemData");
    GiveItem(PC, StoneDef, 500, 0);

    static UFortItemDefinition* MetalDef = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/MetalItemData.MetalItemData");
    GiveItem(PC, MetalDef, 500, 0);

}