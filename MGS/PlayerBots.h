#pragma once
#include "Utils.h"
#include "Gui.h"
#include "InventoryBots.h"
#include <vector>
#include "BotDisplayNames.h"
#include "BotBehaviors.h"


inline AFortAthenaMutator_Bots* BotMutator = nullptr;

inline vector<UAthenaCharacterItemDefinition*> CIDs{};
inline vector<UAthenaPickaxeItemDefinition*> Pickaxes{};
inline vector<UAthenaBackpackItemDefinition*> Backpacks{};
inline vector<UAthenaGliderItemDefinition*> Gliders{};
inline vector<UAthenaSkyDiveContrailItemDefinition*> Contrails{};
inline vector<UAthenaDanceItemDefinition*> Dances{};

struct Bot
{
public:
    AFortPlayerPawnAthena* Pawn = nullptr;
    ABP_PhoebePlayerController_C* PC = nullptr;
    AFortPlayerStateAthena* PlayerState = nullptr;
    EBotState State = EBotState::Warmup;
    EBotState PreviousState = EBotState::MAX;
    bool Tick = false;
    bool bShouldChangeDirection = false;
    float TimeSinceLastChange = 0.0f;
    float DirectionChangeInterval = 3.0f;
    AActor* Loc = nullptr;

    AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
    AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

public:

    Bot(AActor* SpawnLocator)
    {
        SpawnBot(SpawnLocator);
    }

    void SpawnBot(AActor* SpawnLocator)
    {
        if (!Botlobbies || !SpawnLocator)
            return;

        auto CID = CIDs[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, CIDs.size() - 1)];
        auto Backpack = Backpacks[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Backpacks.size() - 1)];
        auto Glider = Gliders[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Gliders.size() - 1)];
        auto Contrail = Contrails[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Contrails.size() - 1)];

        if (!CID || !CID->HeroDefinition || !Backpack || !Glider || !Contrail)
            return;

        static auto BotBP = StaticLoadObject<UClass>("/Game/Athena/AI/Phoebe/BP_PlayerPawn_Athena_Phoebe.BP_PlayerPawn_Athena_Phoebe_C");
        static UBehaviorTree* BehaviorTree = StaticLoadObject<UBehaviorTree>("/Game/Athena/AI/Phoebe/BehaviorTrees/BT_Phoebe.BT_Phoebe");

        if (!BotBP || !BehaviorTree)
            return;

        static auto BotMutator = (AFortAthenaMutator_Bots*)GameMode->ServerBotManager->CachedBotMutator;

        Pawn = BotMutator->SpawnBot(BotBP, SpawnLocator, SpawnLocator->K2_GetActorLocation(), SpawnLocator->K2_GetActorRotation(), false);

        if (!Pawn || !Pawn->Controller)
            return;

        ABP_PhoebePlayerController_C* PC = Cast<ABP_PhoebePlayerController_C>(Pawn->Controller);
        AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(PC->PlayerState);

        if (!PC || !PlayerState)
            return;

        auto PickDef = Pickaxes[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Pickaxes.size() - 1)];
        if (!PickDef)
        {
            return;
        }

        if (PickDef && PickDef->WeaponDefinition)
        {
            GiveItem(PC, PickDef->WeaponDefinition);
        }

        auto Entry = GetEntry(PC ,PickDef->WeaponDefinition);

        Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid);

        if (BotDisplayNamesIG.size() != 0) {
            std::srand(static_cast<unsigned int>(std::time(0)));
            int randomIndex = std::rand() % BotDisplayNamesIG.size();
            std::string rdName = BotDisplayNamesIG[randomIndex];
            BotDisplayNamesIG.erase(BotDisplayNamesIG.begin() + randomIndex);

            int size_needed = MultiByteToWideChar(CP_UTF8, 0, rdName.c_str(), (int)rdName.size(), NULL, 0);
            std::wstring wideString(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, rdName.c_str(), (int)rdName.size(), &wideString[0], size_needed);


            FString CVName = FString(wideString.c_str());
            GameMode->ChangeName(PC, CVName, true);

            PlayerState->OnRep_PlayerName();
        }

        if (CID->HeroDefinition)
        {
            if (CID->HeroDefinition->Specializations.IsValid())
            {
                for (size_t i = 0; i < CID->HeroDefinition->Specializations.Num(); i++)
                {
                    UFortHeroSpecialization* Spec = StaticLoadObject<UFortHeroSpecialization>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(CID->HeroDefinition->Specializations[i].ObjectID.AssetPathName).ToString());
                    if (Spec)
                    {
                        for (size_t j = 0; j < Spec->CharacterParts.Num(); j++)
                        {
                            UCustomCharacterPart* Part = StaticLoadObject<UCustomCharacterPart>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(Spec->CharacterParts[j].ObjectID.AssetPathName).ToString());
                            if (Part)
                            {
                                PlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
                            }
                        }
                    }
                }
            }
        }

        for (size_t j = 0; j < Backpack->CharacterParts.Num(); j++)
        {
            UCustomCharacterPart* Part = Backpack->CharacterParts[j];
            if (Part)
            {
                PlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
            }
        }

        PC->CosmeticLoadoutBC.Glider = Glider;
        PC->CosmeticLoadoutBC.SkyDiveContrail = Contrail;
        Pawn->CosmeticLoadout = PC->CosmeticLoadoutBC;

        UBlackboardData* BlackboardData = StaticLoadObject<UBlackboardData>("/Game/Athena/AI/Phoebe/BehaviorTrees/BB_Phoebe.BB_Phoebe");

        for (auto SkillSet : PC->DigestedBotSkillSets) 
        {
            if (!SkillSet)
                continue;

            if (auto AimingSkill = Cast<UFortAthenaAIBotAimingDigestedSkillSet>(SkillSet))
                PC->CacheAimingDigestedSkillSet = AimingSkill;

            if (auto HarvestSkill = Cast<UFortAthenaAIBotHarvestDigestedSkillSet>(SkillSet))
                PC->CacheHarvestDigestedSkillSet = HarvestSkill;

            if (auto InventorySkill = Cast<UFortAthenaAIBotInventoryDigestedSkillSet>(SkillSet))
                PC->CacheInventoryDigestedSkillSet = InventorySkill;

            if (auto LootingSkill = Cast<UFortAthenaAIBotLootingDigestedSkillSet>(SkillSet))
                PC->CacheLootingSkillSet = LootingSkill;

            if (auto MovementSkill = Cast<UFortAthenaAIBotMovementDigestedSkillSet>(SkillSet))
                PC->CacheMovementSkillSet = MovementSkill;

            if (auto PerceptionSkill = Cast<UFortAthenaAIBotPerceptionDigestedSkillSet>(SkillSet))
                PC->CachePerceptionDigestedSkillSet = PerceptionSkill;

            if (auto PlayStyleSkill = Cast<UFortAthenaAIBotPlayStyleDigestedSkillSet>(SkillSet))
                PC->CachePlayStyleSkillSet = PlayStyleSkill;
        }

        PC->BehaviorTree = BehaviorTree;
        PC->RunBehaviorTree(BehaviorTree);
        PC->UseBlackboard(BehaviorTree->BlackboardAsset, &PC->Blackboard);
        PC->UseBlackboard(BehaviorTree->BlackboardAsset, &PC->Blackboard1);

        Pawn->SetMaxHealth(100);
        Pawn->SetHealth(100);
        Pawn->SetMaxShield(100);
        Pawn->SetShield(0);

        PC->PathFollowingComponent->MyNavData = ((UAthenaNavSystem*)UWorld::GetWorld()->NavigationSystem)->MainNavData;
        PC->PathFollowingComponent->OnNavDataRegistered(((UAthenaNavSystem*)UWorld::GetWorld()->NavigationSystem)->MainNavData);

        PlayerState->OnRep_CharacterData();

        Pawn->CapsuleComponent->SetGenerateOverlapEvents(true);
        Pawn->CharacterMovement->bCanWalkOffLedges = true;

        Tick = true;
    }

	virtual void TickBot()
	{
		if (!Tick)
			return;

        ABP_PhoebePlayerController_C* BotPC = (ABP_PhoebePlayerController_C*)Pawn->Controller;

        switch (State)
        {
        case Warmup:
            WalkForward(Pawn);
            Run(Pawn, false);
            break;
        case InBus:
            break;
        case SkydivingFromBus:
            break;
        case Landed:
            break;
        case Looting:
            break;
        case MovingToZone:
            break;
        case LookingForPlayers:
            break;
        case Stuck:
        case MAX:
            break;
        default:
            break;
        }
	}

};
inline std::vector<Bot*> SpawnedBots{};