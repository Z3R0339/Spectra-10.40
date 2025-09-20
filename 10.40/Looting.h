#pragma once
#include "framework.h"

namespace Looting
{
	const FFortBaseWeaponStats* GetWeaponStats(UFortItemDefinition* ItemDefinition)
	{
		if (!ItemDefinition)
		{
			//Log(std::format("GetWeaponStats: ItemDefinition={}", ItemDefinition->GetFullName()).c_str());
			return nullptr;
		}

		UFortWeaponRangedItemDefinition* Weapon = (UFortWeaponRangedItemDefinition*)ItemDefinition;
		if (!Weapon)
		{
			Log("GetWeaponStats: Weapon Issue?");
			return nullptr;
		}

		const FDataTableRowHandle& Handle = Weapon->WeaponStatHandle;
		if (!Handle.DataTable) 
		{
			Log("GetWeaponStats: Handle Issue?");
			return nullptr;
		}

		for (const auto& Pair : Handle.DataTable->RowMap)
		{
			if (Pair.Key() == Handle.RowName) return (FFortBaseWeaponStats*)Pair.Value();
		}

		Log("GetWeaponStats: return nullptr");
		return nullptr;
	}

	int GetClipSize(UFortItemDefinition* ItemDefinition)
	{
		const FFortBaseWeaponStats* Stats = GetWeaponStats(ItemDefinition);
		return Stats ? Stats->ClipSize : 0;
	}

	inline float RandomFloatForLoot()
	{
		return static_cast<float>(std::rand() / static_cast<float>(RAND_MAX));
	}

	template<typename T>
	T PickWeightedElement(const std::vector<T>& Elements, std::function<float(const T&)> GetWeight)
	{
		float TotalWeight = 0.0f;
		for (const auto& Element : Elements)
			TotalWeight += GetWeight(Element);

		float Random = RandomFloatForLoot() * TotalWeight;

		for (const auto& Element : Elements)
		{
			Random -= GetWeight(Element);
			if (Random <= 0)
				return Element;
		}

		return Elements.empty() ? T() : Elements[0];
	}

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

	//Credit Marvelco (we ask his permission)
	std::vector<FFortItemEntry> PickLootDrops(FName TierGroupName, int Recursive = 0)
	{
		std::vector<FFortItemEntry> LootDrops;

		if (Recursive >= 5)
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

			static UDataTable* MainLTD = StaticLoadObject<UDataTable>(UKismetStringLibrary::Conv_NameToString(GameState->CurrentPlaylistInfo.BasePlaylist->LootTierData.ObjectID.AssetPathName).ToString());;
			static UDataTable* MainLP = StaticLoadObject<UDataTable>(UKismetStringLibrary::Conv_NameToString(GameState->CurrentPlaylistInfo.BasePlaylist->LootPackages.ObjectID.AssetPathName).ToString());

			if (!MainLTD)
				MainLTD = UObject::FindObject<UDataTable>("DataTable AthenaLootTierData_Client.AthenaLootTierData_Client");

			if (!MainLP)
				MainLP = UObject::FindObject<UDataTable>("DataTable AthenaLootPackages_Client.AthenaLootPackages_Client");

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
			return PickLootDrops(TierGroupName, ++Recursive);

		if (ChosenRowLootTierData->LootPackageCategoryMinArray.Num() != ChosenRowLootTierData->LootPackageCategoryWeightArray.Num() ||
			ChosenRowLootTierData->LootPackageCategoryMinArray.Num() != ChosenRowLootTierData->LootPackageCategoryMaxArray.Num())
			return PickLootDrops(TierGroupName, ++Recursive);

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
			return PickLootDrops(TierGroupName, ++Recursive);

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

		/*std::vector<FFortItemEntry> LootDrops;

		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		UFortPlaylistAthena* Playlist = GameState->CurrentPlaylistInfo.BasePlaylist;

		static UDataTable* LootTierTable = Playlist ? Playlist->LootTierData.Get() : nullptr;
		static UDataTable* LootPackageTable = Playlist ? Playlist->LootPackages.Get() : nullptr;

		if (!LootTierTable) LootTierTable = UObject::FindObject<UDataTable>("DataTable AthenaLootTierData_Client.AthenaLootTierData_Client");
		if (!LootPackageTable) LootPackageTable = UObject::FindObject<UDataTable>("DataTable AthenaLootPackages_Client.AthenaLootPackages_Client");
		if (!LootTierTable || !LootPackageTable) return LootDrops;

		std::vector<FFortLootTierData*> MatchTiers;
		float TotalTierWeight = 0.f;

		for (const auto& Pair : LootTierTable->RowMap)
		{
			FFortLootTierData* Row = (FFortLootTierData*)Pair.Value();
			if (!Row) continue;
			if (Row->TierGroup != TierGroupName) continue;
			if (Row->Weight <= 0.f) continue;

			bool bSkipLevelCheck = Row->MinWorldLevel == -1 && Row->MaxWorldLevel == -1;
			if (!bSkipLevelCheck && (Row->MinWorldLevel > WorldLevel || Row->MaxWorldLevel < WorldLevel)) continue;

			MatchTiers.push_back(Row);
			TotalTierWeight += Row->Weight;
		}

		if (MatchTiers.empty())
		{
			if (bPrint)
				Log("PickLootDrops: MatchTiers is empty");
			return LootDrops;
		}

		FFortLootTierData* PickTier = nullptr;

		if (ForcedLootTier >= 0 && ForcedLootTier < MatchTiers.size())
		{
			PickTier = MatchTiers[ForcedLootTier];
		}
		else
		{
			float RandomWeight = RandomFloatForLoot() * TotalTierWeight;
			for (FFortLootTierData* Tier : MatchTiers)
			{
				if (RandomWeight <= Tier->Weight)
				{
					PickTier = Tier;
					break;
				}
				RandomWeight -= Tier->Weight;
			}
		}

		if (!PickTier || PickTier->NumLootPackageDrops <= 0) return LootDrops;

		std::vector<FFortLootPackageData*> MatchPackages;
		float TotalPackageWeight = 0.f;

		for (const auto& Pair : LootPackageTable->RowMap)
		{
			FFortLootPackageData* Row = (FFortLootPackageData*)Pair.Value();
			if (!Row || Row->LootPackageID != PickTier->LootPackage || Row->Weight <= 0.f) continue;
			MatchPackages.push_back(Row);
			TotalPackageWeight += Row->Weight;
		}

		if (MatchPackages.empty())
		{
			if (bPrint)
				Log("PickLootDrops: MatchPackages is empty");
			return LootDrops;
		}

		for (int i = 0; i < (int)PickTier->NumLootPackageDrops; i++)
		{
			float Rand = RandomFloatForLoot() * TotalPackageWeight;
			FFortLootPackageData* PickedPackage = nullptr;

			for (FFortLootPackageData* PKG : MatchPackages)
			{
				if (!PKG) continue;

				if (Rand < PKG->Weight)
				{
					PickedPackage = PKG;
					break;
				}
				Rand -= PKG->Weight;
			}

			if (!PickedPackage)
			{
				Log("PickedPackage is nullptr");
				continue;
			}

			std::string CallStr = PickedPackage->LootPackageCall.ToString();
			bool bHasCall = CallStr != "None" && CallStr.length() > 0;
			std::string AssetStr = PickedPackage->ItemDefinition.ObjectID.AssetPathName.ToString();
			bool bHasAsset = AssetStr != "None" && AssetStr.length() > 0;

			if (bHasCall && !bHasAsset && Recursive < 3)
			{
				if (bPrint)
					Log(std::format("Recursing into LootPackageCall '{}'", CallStr));

				FName CallName = UKismetStringLibrary::Conv_StringToName(PickedPackage->LootPackageCall);
				std::vector<FFortLootPackageData*> SubPackages;
				float SubTotalWeight = 0.f;

				for (const auto& Pair : LootPackageTable->RowMap)
				{
					FFortLootPackageData* SubPKG = (FFortLootPackageData*)Pair.Value();
					if (!SubPKG || SubPKG->LootPackageID != CallName || SubPKG->Weight <= 0.f) continue;
					SubPackages.push_back(SubPKG);
					SubTotalWeight += SubPKG->Weight;
				}

				if (!SubPackages.empty())
				{
					float SubRand = RandomFloatForLoot() * SubTotalWeight;

					for (FFortLootPackageData* SubPKG : SubPackages)
					{
						if (SubRand < SubPKG->Weight)
						{
							UFortItemDefinition* SubItem = SubPKG->ItemDefinition.Get();

							if (!SubItem)
							{
								FSoftObjectPtr& SubRaw = *(FSoftObjectPtr*)&SubPKG;
								std::string SubPath = SubRaw.ObjectID.AssetPathName.ToString();
								if (!SubPath.empty() && SubPath != "None") SubItem = StaticLoadObject<UFortItemDefinition>(SubPath);
							}

							if (!SubItem) break;

							FFortItemEntry SubEntry{ };
							SubEntry.ItemDefinition = SubItem;
							SubEntry.Count = SubPKG->Count;

							if (SubItem->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
								SubEntry.LoadedAmmo = GetClipSize(SubItem);
							
							LootDrops.push_back(SubEntry);
							break;
						}
						SubRand -= SubPKG->Weight;
					}
				}

				continue;
			}

			UFortItemDefinition* ItemDef = PickedPackage->ItemDefinition.Get();

			if (!ItemDef)
			{
				FSoftObjectPtr& Raw = *(FSoftObjectPtr*)&PickedPackage->ItemDefinition;
				std::string AssetPath = Raw.ObjectID.AssetPathName.ToString();
				if (!AssetPath.empty() && AssetPath != "None") ItemDef = StaticLoadObject<UFortItemDefinition>(AssetPath);
				if (bPrint) Log(std::format("Trying ItemDefinition '{}'", AssetPath));
			}

			if (!ItemDef)
			{
				if (bPrint) Log("ItemDefinition is nullptr even after fallback load");
				continue;
			}

			FFortItemEntry Entry{};
			Entry.ItemDefinition = ItemDef;
			Entry.Count = PickedPackage->Count;

			if (ItemDef->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
				Entry.LoadedAmmo = GetClipSize(ItemDef);

			if (bPrint) Log(std::format("Final Drop -> {}", ItemDef->GetFullName()));

			LootDrops.push_back(Entry);
		}

		return LootDrops;*/
	}

	char SpawnLoot(ABuildingContainer* BuildingContainer)
	{

		if (!BuildingContainer)
			return false;

		if (!BuildingContainer->IsA(ABuildingSMActor::StaticClass()))
			return false;

		std::string ClassName = BuildingContainer->Class->GetName();
		//Log("Container Class: " + ClassName);

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
			auto LootDrops = PickLootDrops(SearchLootTierGroup);

			auto CorrectLocation = BuildingContainer->K2_GetActorLocation() + (BuildingContainer->GetActorForwardVector() * BuildingContainer->LootSpawnLocation_Athena.X) + (BuildingContainer->GetActorRightVector() * BuildingContainer->LootSpawnLocation_Athena.Y) + (BuildingContainer->GetActorUpVector() * BuildingContainer->LootSpawnLocation_Athena.Z);

			for (auto& LootDrop : LootDrops)
			{
				SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, PickupSourceTypeFlags, SpawnSource);
			}

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

				}

			}
		}


		return true;
	}

	void DestroyVending()//Marvelco: i will handle these tomorrow
	{
		UGameplayStatics* Statics = UGameplayStatics::GetDefaultObj();

		TArray<AActor*> Spawners;

		UClass* Class = AB_Athena_VendingMachine_C::StaticClass();

		Statics->GetAllActorsOfClass(UWorld::GetWorld(), Class, &Spawners);

		for (int i = 0; i < Spawners.Num(); i++)
			Spawners[i]->K2_DestroyActor();

		Spawners.Free();
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

		SpawnerClass = StaticLoadObject<UClass>("/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_01.Tiered_Athena_FloorLoot_01_C");
		Statics->GetAllActorsOfClass(UWorld::GetWorld(), SpawnerClass, &FloorLootSpawners);

		for (size_t i = 0; i < FloorLootSpawners.Num(); i++)
		{
			FloorLootSpawners[i]->K2_DestroyActor();
		}

		FloorLootSpawners.Free();
		
	}

	static void (*FloorLootOG)(ABuildingSMActor* Actor);
	void FloorLoot(ABuildingSMActor* Actor)
	{
		if (Actor->IsA(AFortPlayerPawn::StaticClass()))
		{
			Log("are we fucked???");
			return;
		}
		return FloorLootOG(Actor);
	}

	void HookAll()
	{
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x13A91C0), SpawnLoot, nullptr);
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x13CEA00), FloorLoot, reinterpret_cast<void**>(&FloorLootOG)); //"ABuildingSMActor::PostUpdate() Building: %s, AltMeshIdx: %d"
		//Marvelco: basicly i came to the conclution that this func is broken/stripped which explains why when we dont spawnfloorloot it crashes the gs 
		// so the bug where the loot spawns on the player is fixed (Big W for tim for finding the offset)
	}
}