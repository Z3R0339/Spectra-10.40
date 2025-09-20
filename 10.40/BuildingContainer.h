#pragma once
#include "framework.h"
#include "Looting.h"

namespace BuildingContainer {
	char SpawnLoot(ABuildingContainer* BuildingContainer)
	{
		if (!BuildingContainer)
			return false;

		if (!BuildingContainer->IsA(ABuildingSMActor::StaticClass()))
			return false;

		std::string ClassName = BuildingContainer->Class->GetName();

		auto SearchLootTierGroup = BuildingContainer->SearchLootTierGroup;
		EFortPickupSpawnSource SpawnSource = EFortPickupSpawnSource::Unset;

		EFortPickupSourceTypeFlag PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Container;

		static auto Chest = UKismetStringLibrary::Conv_StringToName(L"Loot_Treasure");
		static auto AmmoBox = UKismetStringLibrary::Conv_StringToName(L"Loot_Ammo");
		static auto FloorLoot = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaFloorLoot");
		static auto FloorLoot_Warmup = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaFloorLoot_Warmup");

		if (SearchLootTierGroup == FloorLoot || SearchLootTierGroup == FloorLoot_Warmup)
		{
			PickupSourceTypeFlags = EFortPickupSourceTypeFlag::FloorLoot;
		}

		BuildingContainer->bAlreadySearched = true;
		BuildingContainer->SearchBounceData.SearchAnimationCount++;
		BuildingContainer->OnRep_bAlreadySearched();

		if (SearchLootTierGroup == Chest)
		{
			EFortPickupSourceTypeFlag PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Container;

			SearchLootTierGroup = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaTreasure");
			SpawnSource = EFortPickupSpawnSource::Chest;
		}

		if (SearchLootTierGroup == AmmoBox)
		{
			EFortPickupSourceTypeFlag PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Container;

			SearchLootTierGroup = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaAmmoLarge");
			SpawnSource = EFortPickupSpawnSource::AmmoBox;
		}

		if (ClassName.contains("Tiered_Chest"))
		{
			auto LootDrops = Looting::PickLootDrops(SearchLootTierGroup);

			auto CorrectLocation = BuildingContainer->K2_GetActorLocation() + (BuildingContainer->GetActorForwardVector() * BuildingContainer->LootSpawnLocation_Athena.X) + (BuildingContainer->GetActorRightVector() * BuildingContainer->LootSpawnLocation_Athena.Y) + (BuildingContainer->GetActorUpVector() * BuildingContainer->LootSpawnLocation_Athena.Z);

			for (auto& LootDrop : LootDrops)
			{
				SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, PickupSourceTypeFlags, SpawnSource);
			}

			return true;
		}
		else
		{
			auto LootDrops = Looting::PickLootDrops(SearchLootTierGroup);

			auto CorrectLocation = BuildingContainer->K2_GetActorLocation() + (BuildingContainer->GetActorForwardVector() * BuildingContainer->LootSpawnLocation_Athena.X) + (BuildingContainer->GetActorRightVector() * BuildingContainer->LootSpawnLocation_Athena.Y) + (BuildingContainer->GetActorUpVector() * BuildingContainer->LootSpawnLocation_Athena.Z);

			for (auto& LootDrop : LootDrops)
			{
				if (LootDrop.Count > 0)
				{
					SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, PickupSourceTypeFlags, SpawnSource);
				}
			}
		}

		return true;
	}

	void HookAll()
	{
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x13A91C0), SpawnLoot, nullptr);

		Log("BuildingContainer Hooked!");
	}
}