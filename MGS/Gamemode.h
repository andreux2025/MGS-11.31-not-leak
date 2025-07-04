#pragma once
#include "Utils.h"
#include "Looting.h"
#include "Gui.h"
#include "PlayerBots.h"
#include "Vehicles.h"

inline bool (*ReadyToStartMatchOG)(AFortGameModeAthena* GameMode);
inline bool ReadyToStartMatch(AFortGameModeAthena* GameMode)
{
    ReadyToStartMatchOG(GameMode);

    auto GameState = (AFortGameStateAthena*)GameMode->GameState;

    static bool Initialized = false;
    static bool Listening = false;

    if (!GameMode || !GameState)
    {
        return false;
    }

    if (!Initialized)
    {
        Initialized = true;

        UFortPlaylistAthena* Playlist = UObject::FindObject<UFortPlaylistAthena>(ModeForPlaylist[SelectedIndex].second);

        GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
        GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
        GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
        GameState->CurrentPlaylistId = Playlist->PlaylistId;
        GameState->CurrentPlaylistInfo.MarkArrayDirty(); // make sure you add this to your sdk
        
        AFortGameSessionDedicatedAthena* GameSession = SpawnAActor<AFortGameSessionDedicatedAthena>();

        GameSession->SessionName = UKismetStringLibrary::Conv_StringToName(FString(L"MGS"));
        GameSession->MaxPlayers = 100;
        GameMode->GameSession = GameSession;
        GameMode->FortGameSession = GameSession;
        GameMode->CurrentPlaylistId = Playlist->PlaylistId;

        for (size_t i = 0; i < GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels.Num(); i++)
        {
            bool Success = false;
            ULevelStreamingDynamic::GetDefaultObj()->LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels[i], FVector(), FRotator(), &Success);
            GameState->AdditionalPlaylistLevelsStreamed.Add(GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels[i].ObjectID.AssetPathName);
            Log(GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels[i].ObjectID.AssetPathName.ToString());
        }

        GameState->OnRep_AdditionalPlaylistLevelsStreamed();

    }

    if (!GameState->MapInfo)
    {
        return false;
    }

    if (!Listening)
    {
        Listening = true;

        if (Initialized)
        {
            GameState->OnRep_CurrentPlaylistId();
            GameState->OnRep_CurrentPlaylistInfo();
        }

        GameMode->ServerBotManager = (UFortServerBotManagerAthena*)UGameplayStatics::SpawnObject(UFortServerBotManagerAthena::StaticClass(), GameMode);
        GameMode->ServerBotManager->CachedGameMode = GameMode;
        GameMode->ServerBotManager->CachedGameState = GameState;
        GameMode->ServerBotManager->CachedBotMutator = SpawnAActor<AFortAthenaMutator_Bots>();
        GameMode->ServerBotManager->CachedBotMutator->CachedGameMode = GameMode;
        GameMode->ServerBotManager->CachedBotMutator->CachedGameState = GameState;

        FName GameNetDriver = UKismetStringLibrary::Conv_StringToName(FString(L"GameNetDriver"));

        UNetDriver* Driver = CreateNetDriver(UEngine::GetEngine(), UWorld::GetWorld(), GameNetDriver);

        Driver->World = UWorld::GetWorld();
        Driver->NetDriverName = GameNetDriver;

        FString Error;
        FURL URL = FURL();
        URL.Port = 7777;

        InitListen(Driver, UWorld::GetWorld(), URL, false, Error);
        SetWorld(Driver, UWorld::GetWorld());

        UWorld::GetWorld()->NetDriver = Driver;
        UWorld::GetWorld()->LevelCollections[0].NetDriver = Driver;
        UWorld::GetWorld()->LevelCollections[1].NetDriver = Driver;

        CIDs = GetAllObjectsOfClass<UAthenaCharacterItemDefinition>();
        Pickaxes = GetAllObjectsOfClass<UAthenaPickaxeItemDefinition>();
        Backpacks = GetAllObjectsOfClass<UAthenaBackpackItemDefinition>();
        Gliders = GetAllObjectsOfClass<UAthenaGliderItemDefinition>();
        Contrails = GetAllObjectsOfClass<UAthenaSkyDiveContrailItemDefinition>();
        Dances = GetAllObjectsOfClass<UAthenaDanceItemDefinition>();

        GameMode->bWorldIsReady = true;

        auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

        SetConsoleTitleA("MGS hosting on port 7777");
        Log("Listening on port 7777.");

    }

    return UWorld::GetWorld()->NetDriver->ClientConnections.Num() > 0;

}


inline APawn* SpawnDefaultPawnFor(AFortGameMode* GameMode, AFortPlayerController* Player, AActor* StartingLoc)
{
    FTransform Transform = StartingLoc->GetTransform();
    return (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(Player, Transform);
}

inline void (*DispatchRequestOG)(__int64 a1, unsigned __int64* a2, int a3);
inline void DispatchRequest(__int64 a1, unsigned __int64* a2, int a3)
{
    return DispatchRequestOG(a1, a2, 3);
}

inline __int64 PickTeam(__int64 a1, unsigned __int8 a2, __int64 a3)
{
    return 3;
};

void (*StormOG)(AFortGameModeAthena* GameMode, int32 ZoneIndex);
void __fastcall Storm(AFortGameModeAthena* GameMode, int32 ZoneIndex)
{
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;

    static bool First = true;

    /*if (LateGame && !First)
    {
        for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
        {
            GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeID_SurviveStormCircle.AccoladeID_SurviveStormCircle"));
        }
    }
    else if (!LateGame)
    {
        for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
        {
            GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeID_SurviveStormCircle.AccoladeID_SurviveStormCircle"));
        }
    }*/

    static bool initstorm = false;

    if (LateGame && !initstorm)
    {
        initstorm = true;
        GameMode->SafeZonePhase = 2;
        GameState->SafeZonePhase = 2;
        GameState->SafeZonesStartTime = 0;
        ZoneIndex = 3;
        First = false;
    }

    if (LateGame && initstorm)
    {
        int newPhase = GameState->SafeZonePhase + 1;

        GameMode->SafeZonePhase = newPhase;
        GameState->SafeZonePhase = newPhase;
    }


    return StormOG(GameMode, ZoneIndex);
}

inline static __int64 (*StartAircraftPhase)(AFortGameModeAthena* GameMode, char a2) = nullptr;
inline __int64 StartAircraftPhaseHook(AFortGameModeAthena* GM, char a2)
{
    return StartAircraftPhase(GM, a2);
}

//it litterly doesnt work on any func except this
inline static void (*HandleStartingNewPlayerOG)(AFortGameModeAthena*, APlayerController*);
inline void HandleStartingNewPlayer(AFortGameModeAthena* GameMode, APlayerController* NewPlayer)
{
    auto PlayerController = reinterpret_cast<AFortPlayerControllerAthena*>(NewPlayer);
    auto PlayerState = reinterpret_cast<AFortPlayerState*>(PlayerController->PlayerState);

    PlayerState->WorldPlayerId = PlayerController->PlayerState->PlayerID;

    static bool First = false;
    if (!First)
    {
        First = true;

        UObject* SnowSetup = StaticFindObject<UObject>(L"/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations:PersistentLevel.BP_ApolloSnowSetup_2");

        if (SnowSetup)
        {
            UFloatProperty* Prop = StaticFindObject<UFloatProperty>(L"/Game/Athena/Apollo/Environments/Blueprints/CalendarEvents/BP_ApolloSnowSetup.BP_ApolloSnowSetup_C:SnowAmount");
            UFloatProperty* Prop2 = StaticFindObject<UFloatProperty>(L"/Game/Athena/Apollo/Environments/Blueprints/CalendarEvents/BP_ApolloSnowSetup.BP_ApolloSnowSetup_C:SnowFalling");

            if (Prop && Prop2)
            {
                *(float*)(__int64(SnowSetup) + Prop->Offset) = 100.f;
                *(float*)(__int64(SnowSetup) + Prop2->Offset) = 69.f;

                UFunction* Func = SnowSetup->Class->GetFunction("BP_ApolloSnowSetup_C", "OnRep_Snow_Amount");
                UFunction* Func2 = SnowSetup->Class->GetFunction("BP_ApolloSnowSetup_C", "OnRep_SnowFalling");

                if (Func)
                {
                    bool HasAuthority = true;
                    SnowSetup->ProcessEvent(Func, &HasAuthority);
                    SnowSetup->ProcessEvent(Func2, &HasAuthority);
                }
            }
        }
    }

    return HandleStartingNewPlayerOG(GameMode, NewPlayer);
}

//proper trust
inline __int64 (*PEOG)(void*, void*, void*);
inline __int64 PE(UObject* Obj, UFunction* Func, void* Params)
{
    if (Func && Func->GetName() == "ServerAttemptExitVehicle")
    {
        AFortPlayerControllerZone* PC = Cast<AFortPlayerControllerZone>(Obj);
        PEOG(Obj, Func, Params);
        ServerAttemptExitVehicle(PC);

        return false;
    }

    else if (Func && Func->GetName() == "ServerRequestSeatChange")
    {
        AFortPlayerControllerZone* PC = Cast<AFortPlayerControllerZone>(Obj);
        PEOG(Obj, Func, Params);
        ServerAttemptExitVehicle(PC);

        return false;
    }

    return PEOG(Obj, Func, Params);
}
