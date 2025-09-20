#pragma once
#include "framework.h"
#include "FortInventory.h"

namespace Pawn
{
	void ServerHandlePickup(AFortPlayerPawn* PlayerPawn, AFortPickup* Pickup, float InFlyTime, const struct FVector& InStartDirection, bool bPlayPickupSound)
	{
		if (!PlayerPawn || !Pickup) return;
		if (Pickup->bPickedUp) return;

		FFortPickupLocationData& PickupLocationData = Pickup->PickupLocationData;
		FFortItemEntry& PickupItemEntry = Pickup->PrimaryPickupItemEntry;

		PickupLocationData.PickupTarget = PlayerPawn;
		PickupLocationData.ItemOwner = PlayerPawn;
		PickupLocationData.FlyTime = 0.40f; //InFlyTime;
		PickupLocationData.StartDirection = (FVector_NetQuantizeNormal)InStartDirection;
		PickupLocationData.bPlayPickupSound = bPlayPickupSound;
		Pickup->OnRep_PickupLocationData();

		Pickup->bPickedUp = true;
		Pickup->OnRep_bPickedUp();

		PlayerPawn->IncomingPickups.Add(Pickup);
	}

	void (*NetMulticast_Athena_BatchedDamageCuesOG)(AFortPlayerPawn* PlayerPawn, const FAthenaBatchedDamageGameplayCues_Shared& SharedData, const FAthenaBatchedDamageGameplayCues_NonShared& NonSharedData);
	void NetMulticast_Athena_BatchedDamageCues(AFortPlayerPawn* PlayerPawn, const FAthenaBatchedDamageGameplayCues_Shared& SharedData, const FAthenaBatchedDamageGameplayCues_NonShared& NonSharedData)
	{
		if (!PlayerPawn || !PlayerPawn->Controller) return NetMulticast_Athena_BatchedDamageCuesOG(PlayerPawn, SharedData, NonSharedData);

		AFortPlayerController* PC = (AFortPlayerController*)PlayerPawn->Controller;
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		if (!PlayerState || PlayerState->bIsABot) return NetMulticast_Athena_BatchedDamageCuesOG(PlayerPawn, SharedData, NonSharedData);

		if (PlayerPawn->CurrentWeapon)
		{
			FFortItemEntry* ItemEntry = FortInventory::FindItemEntry(PC, PlayerPawn->CurrentWeapon->ItemEntryGuid);
			if (!ItemEntry) return NetMulticast_Athena_BatchedDamageCuesOG(PlayerPawn, SharedData, NonSharedData);
			ItemEntry->LoadedAmmo = PlayerPawn->CurrentWeapon->AmmoCount;
			FortInventory::Update(PC, ItemEntry);
		}

		return NetMulticast_Athena_BatchedDamageCuesOG(PlayerPawn, SharedData, NonSharedData);
	}

	void (*OnCapsuleBeginOverlapOG)(AFortPlayerPawn* Pawn, UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	void OnCapsuleBeginOverlap(AFortPlayerPawn* Pawn, UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
	{
		//Log("OnCapsuleBeginOverlap Called!");
		if (!Pawn || !OtherActor) return;

		if (OtherActor->IsA(AFortPickup::StaticClass()))
		{
			auto PC = (AFortPlayerControllerAthena*)Pawn->Controller;
			if (!PC) return;
			if (PC->PlayerState->bIsABot) return;

			AFortPickup* Pickup = (AFortPickup*)OtherActor;

			if (Pickup->PawnWhoDroppedPickup == Pawn)
				return;

			UFortWorldItemDefinition* Def = (UFortWorldItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition;

			if (!Def) {
				return;
			}

			FFortItemEntry* ItemEntry = FortInventory::FindItemEntry(PC, Def);
			auto Count = ItemEntry ? ItemEntry->Count : 0;

			if (Def->IsStackable()) {
				if (Def->IsA(UFortAmmoItemDefinition::StaticClass()) || Def->IsA(UFortResourceItemDefinition::StaticClass()) || Def->IsA(UFortTrapItemDefinition::StaticClass())) {
					if (Count < Def->MaxStackSize) {
						Pawn->ServerHandlePickup(Pickup, 0.40f, FVector(), true);
					}
				}
				else if (ItemEntry) {
					if (Count < Def->MaxStackSize) {
						Pawn->ServerHandlePickup(Pickup, 0.40f, FVector(), true);
					}
				}
			}
		}

		return;
	}

	void ServerMove(AFortPhysicsPawn* PhysicsPawn, const FReplicatedPhysicsPawnState& InState)
	{
		if (!PhysicsPawn) return;

		if (UPrimitiveComponent* RootComponent = Cast<UPrimitiveComponent>(PhysicsPawn->RootComponent))
		{
			FRotator RealRotation = Rotator(InState.Rotation);

			RealRotation.Yaw = FRotator::UnwindDegrees(RealRotation.Yaw);

			RealRotation.Pitch = 0;
			RealRotation.Roll = 0;

			RootComponent->K2_SetWorldLocationAndRotation(InState.Translation, RealRotation, false, nullptr, true);
			RootComponent->SetPhysicsLinearVelocity(InState.LinearVelocity, 0, FName(0));
			RootComponent->SetPhysicsAngularVelocityInDegrees(InState.AngularVelocity, 0, FName(0));
		}
	}

	void (*ServerSendZiplineStateOG)(AFortPlayerPawn* PlayerPawn, const FZiplinePawnState& InZiplineState);
	void ServerSendZiplineState(AFortPlayerPawn* PlayerPawn, const FZiplinePawnState& InZiplineState)
	{
		if (!PlayerPawn || !PlayerPawn->HasAuthority()) return ServerSendZiplineStateOG(PlayerPawn, InZiplineState);

		PlayerPawn->ZiplineState = InZiplineState;
		OnRep_ZiplineState(PlayerPawn);

		if (!InZiplineState.bIsZiplining)
		{
			if (InZiplineState.bJumped)
			{
				const float JumpZVelocity = 1500.0f;
				const FVector LaunchVelocity = FVector{ 0.f,0.f,JumpZVelocity };

				if (PlayerPawn->CharacterMovement)
				{
					UCharacterMovementComponent* CharacterMovement = PlayerPawn->CharacterMovement;
					
					CharacterMovement->Velocity = FVector();
					CharacterMovement->StopMovementImmediately();
					PlayerPawn->LaunchCharacter(LaunchVelocity, true, true);
				}
			}
		}

		return ServerSendZiplineStateOG(PlayerPawn, InZiplineState);
	}

	void MovingEmoteStopped(AFortPawn* Pawn)
	{
		Log("MovingEmoteStopped Called!");
		if (!Pawn)
			return;

		Pawn->bMovingEmote = false;
	}

	void HookAll()
	{
		HookVTable<APlayerPawn_Athena_C>(0x1C7, ServerHandlePickup);

		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x1EF0A80), NetMulticast_Athena_BatchedDamageCues, reinterpret_cast<void**>(&NetMulticast_Athena_BatchedDamageCuesOG));
		MH_CreateHook((LPVOID)(ImageBase + 0x196DB00), OnCapsuleBeginOverlap, (LPVOID*)&OnCapsuleBeginOverlapOG);
		//MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x1EFAC60), ServerMove, nullptr);

		HookVTable<APlayerPawn_Athena_C>(0x1D2, ServerSendZiplineState, reinterpret_cast<void**>(&ServerSendZiplineStateOG));

		//MH_CreateHook((LPVOID)(ImageBase + 0xB3EBA0), MovingEmoteStopped, nullptr);

		Log("Pawn Hooked!");
	}
}