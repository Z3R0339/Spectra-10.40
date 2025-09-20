#pragma once
#include "framework.h"

#include "ServerBotManager.h"
#include "FortInventory.h"

namespace FortPickup {
	__int64 (*CompletePickupAnimationOG)(AFortPickup* Pickup);
	__int64 CompletePickupAnimation(AFortPickup* Pickup)
	{
		if (!Pickup) {
			Log("No Pickup!");
			return CompletePickupAnimationOG(Pickup);
		}

		FFortPickupLocationData& PickupLocationData = Pickup->PickupLocationData;
		FFortItemEntry& PickupEntry = Pickup->PrimaryPickupItemEntry;

		AFortPlayerPawnAthena* Pawn = (AFortPlayerPawnAthena*)PickupLocationData.PickupTarget;
		if (!Pawn) {
			Log("No Pawn!");
			return CompletePickupAnimationOG(Pickup);
		}

		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;
		if (!PC) return CompletePickupAnimationOG(Pickup);
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		if (!PlayerState) return CompletePickupAnimationOG(Pickup);
		if (PlayerState->bIsABot) {
			for (PlayerBots::PlayerBot* Bot : PlayerBots::PlayerBotArray) {
				if (Bot->Pawn == Pawn) {
					PlayerBots::BotsBTService_InventoryManager InventoryManager;
					InventoryManager.CompletePickupAnimation(Bot, Pickup);
					break;
				}
			}
			return CompletePickupAnimationOG(Pickup);
		}

		UFortItemDefinition* PickupItemDefinition = PickupEntry.ItemDefinition;

		int PickupCount = PickupEntry.Count;
		int PickupLoadedAmmo = PickupEntry.LoadedAmmo;
		int PickupMaxStackSize = PickupItemDefinition->MaxStackSize;
		if (!PC->WorldInventory) return CompletePickupAnimationOG(Pickup);
		FFortItemEntry* ItemEntry = FortInventory::FindItemEntry(PC, PickupItemDefinition);

		AFortWeapon* CurrentWeapon = Pawn->CurrentWeapon;
		if (!CurrentWeapon || !CurrentWeapon->WeaponData) {
			return CompletePickupAnimationOG(Pickup);
		}

		FVector Drop = Pawn->K2_GetActorLocation() + Pawn->GetActorForwardVector() * 100.f;
		if (ItemEntry) {
			if (PickupItemDefinition->IsStackable()) {
				bool bCanPickup = (ItemEntry->Count + PickupCount) <= PickupMaxStackSize;
				if (PickupItemDefinition->IsA(UFortTrapItemDefinition::StaticClass())) {
					bCanPickup = true;
				}
				if (bCanPickup) {
					FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, PickupLoadedAmmo, true);
				}
				else {
					int Space = PickupMaxStackSize - ItemEntry->Count;
					int AddToStack = UKismetMathLibrary::GetDefaultObj()->min_0(Space, PickupCount);
					int LeftOver = PickupCount - AddToStack;

					if (AddToStack > 0) {
						FortInventory::GiveItem(PC, PickupItemDefinition, AddToStack, 0, true);
						SpawnPickup(PickupItemDefinition, LeftOver, PickupLoadedAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn);
					}
					else {
						if (FortInventory::GetQuickBars(CurrentWeapon->WeaponData) == EFortQuickBars::Primary) {
							FFortItemEntry* CurrentWeaponItemEntry = FortInventory::FindItemEntry(PC, CurrentWeapon->WeaponData);

							SpawnPickup(CurrentWeapon->WeaponData, CurrentWeaponItemEntry->Count, CurrentWeaponItemEntry->LoadedAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn);
							FortInventory::RemoveItem(PC, CurrentWeapon->WeaponData);
							FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, 0, false);
						}
						else {
							SpawnPickup(PickupItemDefinition, PickupCount, PickupLoadedAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn);
						}
					}
				}
			}
			else {
				if (FortInventory::GetQuickBars(CurrentWeapon->WeaponData) == EFortQuickBars::Primary) {
					if (FortInventory::IsFullInventory(PC)) {
						FFortItemEntry* CurrentWeaponItemEntry = FortInventory::FindItemEntry(PC, CurrentWeapon->WeaponData);

						SpawnPickup(CurrentWeapon->WeaponData, CurrentWeaponItemEntry->Count, CurrentWeaponItemEntry->LoadedAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn);
						FortInventory::RemoveItem(PC, CurrentWeapon->WeaponData);
						FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, 0);
					}
					else {
						FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, 0);
					}
				}
				else {
					SpawnPickup(PickupItemDefinition, PickupCount, PickupLoadedAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn);
				}
			}
		}
		else {
			if (PickupItemDefinition->IsStackable()) {
				if (PickupItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || PickupItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) ||
					PickupItemDefinition->IsA(UFortTrapItemDefinition::StaticClass())) {
					FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, 0);
				}
				else {
					if (FortInventory::GetQuickBars(CurrentWeapon->WeaponData) == EFortQuickBars::Primary) {
						if (FortInventory::IsFullInventory(PC)) {
							FFortItemEntry* CurrentWeaponItemEntry = FortInventory::FindItemEntry(PC, CurrentWeapon->WeaponData);

							SpawnPickup(CurrentWeapon->WeaponData, CurrentWeaponItemEntry->Count, CurrentWeaponItemEntry->LoadedAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn);
							FortInventory::RemoveItem(PC, CurrentWeapon->WeaponData);
							FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, 0);
						}
						else {
							FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, 0);
						}
					}
					else {
						if (FortInventory::IsFullInventory(PC)) {
							SpawnPickup(PickupItemDefinition, PickupCount, PickupLoadedAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn);
						}
						else {
							FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, 0);
						}
					}
				}
			}
			else {
				if (PickupItemDefinition->IsA(UFortTrapItemDefinition::StaticClass())) {
					FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, PickupLoadedAmmo);
				}
				else if (FortInventory::IsFullInventory(PC)) {
					if (FortInventory::GetQuickBars(CurrentWeapon->WeaponData) == EFortQuickBars::Primary) {
						FFortItemEntry* CurrentWeaponItemEntry = FortInventory::FindItemEntry(PC, CurrentWeapon->WeaponData);

						SpawnPickup(CurrentWeapon->WeaponData, CurrentWeaponItemEntry->Count, CurrentWeaponItemEntry->LoadedAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn);
						FortInventory::RemoveItem(PC, CurrentWeapon->WeaponData);
						FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, PickupLoadedAmmo);
					}
					else {
						SpawnPickup(PickupItemDefinition, PickupCount, PickupLoadedAmmo, Drop, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn);
					}
				}
				else {
					FortInventory::GiveItem(PC, PickupItemDefinition, PickupCount, PickupLoadedAmmo);
				}
			}
		}

		FortInventory::Update(PC);

		Pickup->K2_DestroyActor();
		return CompletePickupAnimationOG(Pickup);
	}

	void HookAll() {
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x16F7D10), CompletePickupAnimation, reinterpret_cast<void**>(&CompletePickupAnimationOG));

		Log("FortPickup Hooked!");
	}
}