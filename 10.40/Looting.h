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
}