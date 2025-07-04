#pragma once
#include <string>
#include "Utils.h"
#include <vector>
#include <map>

//straight from the ue src 

static void(*GiveAbility)(SDK::UAbilitySystemComponent*, SDK::FGameplayAbilitySpecHandle*, SDK::FGameplayAbilitySpec) = decltype(GiveAbility)(__int64(GetModuleHandleW(0)) + 0xB76E70);
static void (*AbilitySpecCtor)(SDK::FGameplayAbilitySpec*, SDK::UGameplayAbility*, int, int, SDK::UObject*) = decltype(AbilitySpecCtor)(__int64(GetModuleHandleW(0)) + 0xB9AF40);
inline bool (*InternalTryActivateAbility)(UAbilitySystemComponent* AbilitySystemComp, FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey InPredictionKey, UGameplayAbility** OutInstancedAbility, void* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData) = decltype(InternalTryActivateAbility)(__int64(GetModuleHandleW(0)) + 0xB78580);

inline void InitAbilitiesForPlayer(SDK::AFortPlayerController* PC)
{
    auto PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
    static auto AbilitySet = StaticLoadObject<UFortAbilitySet>("/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer");

    if (PlayerState && AbilitySet)
    {
        for (size_t i = 0; i < AbilitySet->GameplayAbilities.Num(); i++)
        {
            FGameplayAbilitySpec Spec{};
            AbilitySpecCtor(&Spec, (UGameplayAbility*)AbilitySet->GameplayAbilities[i].Get()->DefaultObject, 1, -1, nullptr);
            GiveAbility(PlayerState->AbilitySystemComponent, &Spec.Handle, Spec);
        }
    }
}

inline FGameplayAbilitySpec* FindAbilitySpecFromHandle(UFortAbilitySystemComponentAthena* ASC, FGameplayAbilitySpecHandle& Handle)
{
    for (size_t i = 0; i < ASC->ActivatableAbilities.Items.Num(); i++)
    {
        if (ASC->ActivatableAbilities.Items[i].Handle.Handle == Handle.Handle)
            return &ASC->ActivatableAbilities.Items[i];
    }
    return nullptr;
}


inline void InternalServerTryActivateAbilityHook(UFortAbilitySystemComponentAthena* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, FPredictionKey& PredictionKey, FGameplayEventData* TriggerEventData) // Broo what is this thing
{
    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilitySystemComponent, Handle);
    if (!Spec)
        return AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);

    UGameplayAbility* AbilityToActivate = Spec->Ability;

    UGameplayAbility* InstancedAbility = nullptr;
    Spec->InputPressed = true;

    if (!InternalTryActivateAbility(AbilitySystemComponent, Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
    {
        AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
        Spec->InputPressed = false;
    }
    AbilitySystemComponent->ActivatableAbilities.MarkItemDirty(*Spec);
}

inline void ServerSendZiplineState(AFortPlayerPawn* Pawn, FZiplinePawnState InZiplineState) //Mr timeless is this even proper?
{
    if (InZiplineState.AuthoritativeValue > Pawn->ZiplineState.AuthoritativeValue)
    {
        Pawn->ZiplineState = InZiplineState;
        OnRep_ZiplineState(Pawn);

        if (!Pawn->ZiplineState.bIsZiplining)
        {
            if (Pawn->ZiplineState.bJumped)
            {
                float ZiplineJumpDampening = -0.5f;
                float ZiplineJumpStrength = 1500.f;

                auto CharacterMovement = Pawn->CharacterMovement;
                auto Velocity = CharacterMovement->Velocity;

                FVector LaunchVelocity = { -750, -750, ZiplineJumpStrength };

                if (ZiplineJumpDampening * Velocity.X >= -750.f)
                {
                    LaunchVelocity.X = fminf(ZiplineJumpDampening * Velocity.X, 750);
                }
                if (ZiplineJumpDampening * Velocity.Y >= -750.f)
                {
                    LaunchVelocity.Y = fminf(ZiplineJumpDampening * Velocity.Y, 750);
                }

                Pawn->LaunchCharacter(LaunchVelocity, false, false);
            }
        }
    }
}