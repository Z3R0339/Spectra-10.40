#pragma once
#include "framework.h"

#include "FortInventory.h"

namespace FortWeapon {
	void (*OnReloadOG)(AFortWeapon* This, int RemoveCount);
	void OnReload(AFortWeapon* This, int RemoveCount)
	{
		if (!This) return OnReloadOG(This, RemoveCount);

		AFortPlayerPawn* Pawn = (AFortPlayerPawn*)This->GetOwner();
		if (!Pawn || !Pawn->Controller) return OnReloadOG(This, RemoveCount);

		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;
		if (!PC) return OnReloadOG(This, RemoveCount);
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)Pawn->PlayerState;
		if (!PlayerState || PlayerState->bIsABot) return OnReloadOG(This, RemoveCount);

		FFortItemEntry* WeaponItemEntry = FortInventory::FindItemEntry(PC, This->ItemEntryGuid);
		if (!WeaponItemEntry || !WeaponItemEntry->ItemDefinition) return OnReloadOG(This, RemoveCount);

		UFortWorldItemDefinition* AmmoItemDef = This->WeaponData ? This->WeaponData->GetAmmoWorldItemDefinition_BP() : nullptr;
		if (!AmmoItemDef) return OnReloadOG(This, RemoveCount);

		FFortItemEntry* AmmoItemEntry = FortInventory::FindItemEntry(PC, AmmoItemDef);
		if (AmmoItemEntry)
		{
			FortInventory::RemoveItem(PC, AmmoItemEntry->ItemDefinition, RemoveCount);
		}
		else
		{
			int MaxStackSize = WeaponItemEntry->ItemDefinition->MaxStackSize;
			if (MaxStackSize > 1) FortInventory::RemoveItem(PC, WeaponItemEntry->ItemDefinition, RemoveCount);
		}

		WeaponItemEntry->LoadedAmmo = This->AmmoCount;

		FortInventory::Update(PC, WeaponItemEntry);

		return OnReloadOG(This, RemoveCount);
	}

	void HookAll() {
		MH_CreateHook((LPVOID)(ImageBase + 0x1C66A30), OnReload, (LPVOID*)&OnReloadOG);

		Log("FortWeapon Hooked!");
	}
}