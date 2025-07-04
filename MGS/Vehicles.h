#pragma once
#include"Utils.h"
#include"Inventory.h"

void ServerMove(AActor* real, FReplicatedPhysicsPawnState InState)
{
	UPrimitiveComponent* RootComponent = (UPrimitiveComponent*)real->RootComponent;

	InState.Rotation.X -= 2.5;
	InState.Rotation.Y /= 0.3;
	InState.Rotation.Z -= -2.0;
	InState.Rotation.W /= -1.2;

	FTransform Transform{};
	Transform.Translation = InState.Translation;
	Transform.Rotation = InState.Rotation;
	Transform.Scale3D = FVector{ 1, 1, 1 };

	RootComponent->K2_SetWorldTransform(Transform, false, nullptr, true);
	RootComponent->bComponentToWorldUpdated = true;
	RootComponent->SetPhysicsLinearVelocity(InState.LinearVelocity, 0, FName());
	RootComponent->SetPhysicsAngularVelocity(InState.AngularVelocity, 0, FName());
}

void SpawnVehicles()
{

	AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

	TArray<AActor*> Spawners;
	UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortAthenaVehicleSpawner::StaticClass(), &Spawners);

	for (AActor* Actor : Spawners) {
		AFortAthenaVehicleSpawner* Spawner = Cast<AFortAthenaVehicleSpawner>(Actor);
		if (!Spawner) continue;

		std::string Name = Spawner->GetName();

		if (!Name.starts_with("Athena_Meatball_L_Spawner")) 
			continue;

		AActor* Vehicle = SpawnActorClass<AActor>(Spawner->K2_GetActorLocation(), Spawner->K2_GetActorRotation(), Spawner->GetVehicleClass());
	}

	Spawners.Free();
}

inline void ServerAttemptExitVehicle(AFortPlayerControllerZone* PC)
{
	if (!PC)
		return;

	auto Pawn = (AFortPlayerPawn*)PC->Pawn;

	if (!Pawn->CurrentWeapon || !Pawn->CurrentWeapon->IsA(AFortWeaponRangedForVehicle::StaticClass()))
		return;

	RemoveItem((AFortPlayerController*)Pawn->Controller, Pawn->CurrentWeapon->GetInventoryGUID(), 1);

	UFortWorldItemDefinition* SwappingItemDef = ((AFortPlayerControllerAthena*)PC)->SwappingItemDefinition;
	if (!SwappingItemDef)
		return;

	FFortItemEntry* SwappingItemEntry = FindItemEntry(PC, SwappingItemDef);
	if (!SwappingItemEntry)
		return;

	PC->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)SwappingItemDef, SwappingItemEntry->ItemGuid);
}
