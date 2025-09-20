#pragma once
#include "framework.h"
#include "FortInventory.h"
#include "FortAIBotControllerAthena.h"

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

	void (*OnReloadOG)(AFortWeapon* a1, int a2 /*Amount*/);
	void __fastcall OnReload(AFortWeapon* a1, int a2 /*Amount*/)
	{
		if (!a1) return OnReloadOG(a1, a2);

		AFortPlayerPawn* PlayerPawn = (AFortPlayerPawn*)a1->GetOwner();
		if (!PlayerPawn || !PlayerPawn->Controller) return OnReloadOG(a1, a2);

		AFortPlayerController* PC = (AFortPlayerController*)PlayerPawn->Controller;
		if (!PC) return OnReloadOG(a1, a2);
		AFortPlayerStateAthena* PS = (AFortPlayerStateAthena*)PlayerPawn->PlayerState;
		if (!PS || PS->bIsABot) return OnReloadOG(a1, a2);
		
		FFortItemEntry* WeaponItemEntry = FortInventory::FindItemEntry(PC, a1->ItemEntryGuid);
		if (!WeaponItemEntry || !WeaponItemEntry->ItemDefinition) return OnReloadOG(a1, a2);


		UFortWorldItemDefinition* AmmoDef = a1->WeaponData ? a1->WeaponData->GetAmmoWorldItemDefinition_BP() : nullptr;
		if (!AmmoDef) return OnReloadOG(a1, a2);

		FFortItemEntry* AmmoItemEntry = FortInventory::FindItemEntry(PC, AmmoDef);
		//if (!AmmoItemEntry) return OnReloadOG(a1, a2);

		if (AmmoItemEntry)
		{
			FortInventory::RemoveItem(PC, AmmoItemEntry->ItemDefinition, a2);
		}
		else
		{
			int MaxStackSize = WeaponItemEntry->ItemDefinition->MaxStackSize;
			if (MaxStackSize > 1) FortInventory::RemoveItem(PC, WeaponItemEntry->ItemDefinition, a2);
		}

		WeaponItemEntry->LoadedAmmo = a1->AmmoCount; //a1->GetBulletsPerClip();

		FortInventory::Update(PC, WeaponItemEntry);
		
		return OnReloadOG(a1, a2);
	}


	__int64 (*CompletePickupAnimationOG)(AFortPickup* a1);
	__int64 __fastcall CompletePickupAnimation(AFortPickup* a1)
	{
		if (!a1) return CompletePickupAnimationOG(a1);

		FFortPickupLocationData& PickupLocationData = a1->PickupLocationData;
		FFortItemEntry& PickupEntry = a1->PrimaryPickupItemEntry;

		AFortPlayerPawn* PlayerPawn = (AFortPlayerPawn*)PickupLocationData.PickupTarget;
		if (!PlayerPawn) return CompletePickupAnimationOG(a1);

		AFortPlayerController* PC = (AFortPlayerController*)PlayerPawn->Controller;
		if (!PC) return CompletePickupAnimationOG(a1);
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		if (!PlayerState) return CompletePickupAnimationOG(a1);
		if (PlayerState->bIsABot) {
			for (PlayerBots::PlayerBot* Bot : PlayerBots::PlayerBotArray) {
				if (Bot->Pawn == PlayerPawn) {
					//Log("Found!");
					PlayerBots::BotsBTService_InventoryManager InventoryManager;
					InventoryManager.CompletePickupAnimation(Bot, a1);
					break;
				}
			}
			return CompletePickupAnimationOG(a1);
		}

		UFortItemDefinition* PickupItemDefinition = PickupEntry.ItemDefinition;

		int PickupCount = PickupEntry.Count;
		int PickupLoadedAmmo = PickupEntry.LoadedAmmo;
		int PickupMaxStackSize = PickupItemDefinition->MaxStackSize;
		if (!PC->WorldInventory) return CompletePickupAnimationOG(a1);
		FFortItemEntry* ItemEntry = FortInventory::FindItemEntry(PC, PickupItemDefinition);

		FVector Drop = PlayerPawn->K2_GetActorLocation() + PlayerPawn->GetActorForwardVector() * 100.f;

		if (!ItemEntry && PickupItemDefinition->IsStackable())
		{
			if (FortInventory::IsFullInventory(PC))
			{
				AFortWeapon* CurrentWeapon = PlayerPawn->CurrentWeapon;

				if (CurrentWeapon)
				{
					UFortItemDefinition* OldWeaponDef = CurrentWeapon->WeaponData;

					if (OldWeaponDef && OldWeaponDef->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
					{
						//Log("Issue 1");
						if (FortInventory::GetQuickBars(PickupItemDefinition) == EFortQuickBars::Secondary)
						{
							//Log("true");
							FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, PickupLoadedAmmo);
							FortInventory::Update(PC);
						}
						else
						{
							//Log("False");
							SpawnPickup(PickupItemDefinition, PickupCount, PickupLoadedAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerPawn);
						}
					}
					else
					{
						//Log("Issue 2");
						
						if (FortInventory::GetQuickBars(PickupItemDefinition) == EFortQuickBars::Primary)
						{
							FFortItemEntry* WeaponItemEntry = FortInventory::FindItemEntry(PC, CurrentWeapon->WeaponData);
							if (!WeaponItemEntry) return CompletePickupAnimationOG(a1);

							int OldWeaponCount = WeaponItemEntry->Count;
							int OldWeaponAmmo = WeaponItemEntry->LoadedAmmo;

							SpawnPickup(OldWeaponDef, OldWeaponCount, OldWeaponAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerPawn);

							FortInventory::RemoveItem(PC, OldWeaponDef, OldWeaponCount);
							FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, PickupLoadedAmmo);
							FortInventory::Update(PC);
						}
						else if (FortInventory::GetQuickBars(PickupItemDefinition) == EFortQuickBars::Secondary)
						{
							FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, PickupLoadedAmmo);
							FortInventory::Update(PC);
						}
					}

					a1->K2_DestroyActor();
					return CompletePickupAnimationOG(a1);
				}
				else
				{
					//Log("Issue 3");
					SpawnPickup(PickupItemDefinition, PickupCount, PickupLoadedAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerPawn);

					a1->K2_DestroyActor();
					return CompletePickupAnimationOG(a1);
				}
			}
			else
			{
				//Log("Issue 4");
				FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, PickupLoadedAmmo);
				FortInventory::Update(PC);
				
				a1->K2_DestroyActor();
				return CompletePickupAnimationOG(a1);
			}
		}

		if (PickupItemDefinition->IsStackable() && ItemEntry && ItemEntry->Count < PickupMaxStackSize)
		{
			int Combined = ItemEntry->Count + PickupEntry.Count;
			if (Combined <= PickupMaxStackSize)
			{
				//Log("Issue 5");
				ItemEntry->Count = Combined;
				FortInventory::Update(PC, ItemEntry);
			}
			else
			{
				//Log("Issue 6");
				ItemEntry->Count = PickupMaxStackSize;
				FortInventory::Update(PC, ItemEntry);
				int Overflow = Combined - PickupMaxStackSize;
				FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, PickupLoadedAmmo);
			}

			a1->K2_DestroyActor();
			return CompletePickupAnimationOG(a1);
		}

		if (FortInventory::IsFullInventory(PC))
		{
			AFortWeapon* CurrentWeapon = PlayerPawn->CurrentWeapon;
			
			if (CurrentWeapon)
			{
				UFortItemDefinition* OldWeaponDef = CurrentWeapon->WeaponData;

				if (OldWeaponDef && OldWeaponDef->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
				{
					if (FortInventory::GetQuickBars(PickupItemDefinition) == EFortQuickBars::Secondary)
					{
						FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, PickupLoadedAmmo);
						FortInventory::Update(PC);
					}
					else
					{
						SpawnPickup(PickupItemDefinition, PickupCount, PickupLoadedAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerPawn);
					}
				}
				else
				{
					if (FortInventory::GetQuickBars(PickupItemDefinition) == EFortQuickBars::Primary)
					{
						FFortItemEntry* WeaponItemEntry = FortInventory::FindItemEntry(PC, CurrentWeapon->WeaponData);
						if (!WeaponItemEntry) return CompletePickupAnimationOG(a1);

						int OldWeaponCount = WeaponItemEntry->Count;
						int OldWeaponAmmo = WeaponItemEntry->LoadedAmmo;

						SpawnPickup(OldWeaponDef, OldWeaponCount, OldWeaponAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerPawn);

						FortInventory::RemoveItem(PC, OldWeaponDef, OldWeaponCount);
						FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, PickupLoadedAmmo);
						FortInventory::Update(PC);
					}
					else if (FortInventory::GetQuickBars(PickupItemDefinition) == EFortQuickBars::Secondary)
					{
						FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, PickupLoadedAmmo);
						FortInventory::Update(PC);
					}
				}

				a1->K2_DestroyActor();
				return CompletePickupAnimationOG(a1);
			}
			else
			{
				SpawnPickup(PickupItemDefinition, PickupCount, PickupLoadedAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerPawn);

				a1->K2_DestroyActor();
				return CompletePickupAnimationOG(a1);
			}
		}

		FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, PickupLoadedAmmo);
		FortInventory::Update(PC);

		a1->K2_DestroyActor();
		return CompletePickupAnimationOG(a1);
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

	void HookAll()
	{
		HookVTable<APlayerPawn_Athena_C>(0x1C7, ServerHandlePickup);

		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x1C66A30), OnReload, reinterpret_cast<void**>(&OnReloadOG));
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x16F7D10), CompletePickupAnimation, reinterpret_cast<void**>(&CompletePickupAnimationOG));
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x1EF0A80), NetMulticast_Athena_BatchedDamageCues, reinterpret_cast<void**>(&NetMulticast_Athena_BatchedDamageCuesOG));
		MH_CreateHook((LPVOID)(ImageBase + 0x196DB00), OnCapsuleBeginOverlap, (LPVOID*)&OnCapsuleBeginOverlapOG);
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x1EFAC60), ServerMove, nullptr);

		HookVTable<APlayerPawn_Athena_C>(0x1D2, ServerSendZiplineState, reinterpret_cast<void**>(&ServerSendZiplineStateOG));

		Log("Pawn Hooked!");
	}
}