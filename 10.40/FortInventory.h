#pragma once
#include "framework.h"

namespace FortInventory
{
	static void Update(AFortPlayerController* PC, FFortItemEntry* ItemEntry = nullptr)
	{
		PC->HandleWorldInventoryLocalUpdate();
		PC->WorldInventory->HandleInventoryLocalUpdate();
		PC->WorldInventory->bRequiresLocalUpdate = true;
		PC->WorldInventory->ForceNetUpdate();
		if (ItemEntry == nullptr)
			PC->WorldInventory->Inventory.MarkArrayDirty();
		else
			PC->WorldInventory->Inventory.MarkItemDirty(*ItemEntry);
	}

	static UFortWorldItem* CreateNewItem(AFortPlayerController* PC, UFortItemDefinition* ItemDefinition, int Count)
	{
		UFortWorldItem* NewItem = (UFortWorldItem*)ItemDefinition->CreateTemporaryItemInstanceBP(Count, 1);
		NewItem->SetOwningControllerForTemporaryItem(PC);
		return NewItem;
	}

	void GiveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count = 1, int LoadedAmmo = 1, bool bShouldAddToExistingStack = false)
	{
		if (bShouldAddToExistingStack) {
			for (FFortItemEntry& ItemEntry : PC->WorldInventory->Inventory.ReplicatedEntries) {
				if (Def == ItemEntry.ItemDefinition) {
					ItemEntry.Count += Count;

					PC->WorldInventory->Inventory.MarkItemDirty(ItemEntry);
					Update(PC);
					break;
				}
			}
			return;
		}

		UFortWorldItem* Item = Cast<UFortWorldItem>(Def->CreateTemporaryItemInstanceBP(Count, 0));
		Item->SetOwningControllerForTemporaryItem(PC);
		Item->ItemEntry.LoadedAmmo = LoadedAmmo;

		/*if (Item && Item->ItemEntry.ItemDefinition) {
			FFortItemEntryStateValue Value{};
			Value.IntValue = true;
			Value.StateType = EFortItemEntryState::ShouldShowItemToast;
			Item->ItemEntry.StateValues.Add(Value);
		}*/

		PC->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		PC->WorldInventory->Inventory.ItemInstances.Add(Item);
		PC->WorldInventory->Inventory.MarkItemDirty(Item->ItemEntry);
		PC->WorldInventory->HandleInventoryLocalUpdate();
	}

	static UFortItemDefinition* FindItemDefinition(AFortPlayerController* PC, const FGuid& Guid)
	{
		if (!PC) return nullptr;

		for (int i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			FFortItemEntry* ItemEntries = &PC->WorldInventory->Inventory.ReplicatedEntries[i];
			if (!ItemEntries) continue;
			if (ItemEntries->ItemGuid == Guid) return ItemEntries->ItemDefinition;
		}

		return nullptr;
	}

	static FFortItemEntry* FindItemEntry(AFortPlayerController* PC, UFortItemDefinition* ItemDefinition)
	{
		if (!PC || !ItemDefinition) return nullptr;
		if (!PC->WorldInventory) return nullptr;

		for (int i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			FFortItemEntry* ItemEntry = &PC->WorldInventory->Inventory.ReplicatedEntries[i];
			if (!ItemEntry) continue;
			if (ItemEntry->ItemDefinition == ItemDefinition) return ItemEntry;
		}

		return nullptr;
	}

	static FFortItemEntry* FindItemEntry(AFortPlayerController* PC, const FGuid& Guid)
	{
		if (!PC) return nullptr;

		for (int i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			FFortItemEntry* ItemEntry = &PC->WorldInventory->Inventory.ReplicatedEntries[i];
			if (!ItemEntry) continue;
			if (ItemEntry->ItemGuid == Guid) return ItemEntry;
		}

		return nullptr;
	}

	static void RemoveItem(AFortPlayerController* PC, UFortItemDefinition* ItemDefinition, int Count = 1)
	{
		if (!PC || !ItemDefinition) return;

		for (int i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			FFortItemEntry* ItemEntry = &PC->WorldInventory->Inventory.ReplicatedEntries[i];
			if (!ItemEntry) continue;

			if (ItemEntry->ItemDefinition == ItemDefinition)
			{
				if (ItemEntry->Count > Count)
				{
					ItemEntry->Count -= Count;
					Update(PC, ItemEntry);
				}
				else
				{
					PC->WorldInventory->Inventory.ReplicatedEntries.Remove(i);
					Update(PC);
				}

				break;
			}
		}
	}

	static EFortQuickBars GetQuickBars(UFortItemDefinition* ItemDefinition)
	{
		if (!ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) && 
			!ItemDefinition->IsA(UFortEditToolItemDefinition::StaticClass()) &&
			!ItemDefinition->IsA(UFortBuildingItemDefinition::StaticClass()) && 
			!ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) && 
			!ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) && 
			!ItemDefinition->IsA(UFortTrapItemDefinition::StaticClass()))
			return EFortQuickBars::Primary;

		return EFortQuickBars::Secondary;
	}

	static bool IsFullInventory(AFortPlayerController* PC)
	{
		if (!PC || !PC->WorldInventory) return true;
		
		static constexpr int Max = 5;
		int Count = 0;

		for (int i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			FFortItemEntry* Entries = &PC->WorldInventory->Inventory.ReplicatedEntries[i];
			if (!Entries) continue;
			if (!Entries->ItemDefinition) continue;

			if (GetQuickBars(Entries->ItemDefinition) == EFortQuickBars::Primary)
			{
				++Count;
				//Log(std::format("Count={}", Count).c_str());
				if (Count >= Max)
					return true;
			}
		}

		return false;
	}
}