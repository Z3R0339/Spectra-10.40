#pragma once
#include "framework.h"
#include "FortInventory.h"
#include "AbilitySystemComponent.h"
#include "Looting.h"
#include "Vehicles.h"
#include "ServerBotManager.h"
#include "GameMode.h"

namespace Controller
{
	void (*ServerAcknowledgePossessionOG)(APlayerController* PC, APawn* P);
	void ServerAcknowledgePossession(APlayerController* PC, APawn* P)
	{
		PC->AcknowledgedPawn = P;

		return ServerAcknowledgePossessionOG(PC, P);
	}

	void (*ServerReadyToStartMatchOG)(AFortPlayerControllerAthena* PC);
	void ServerReadyToStartMatch(AFortPlayerControllerAthena* PC)
	{
		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto GameState = (AFortGameStateAthena*)GameMode->GameState;

		static bool bSetupWorld = false;

		if (!bSetupWorld)
		{
			bSetupWorld = true;

			Looting::SpawnFloorLoot();
			Vehicles::SpawnVehicles();

			Log(std::format("AFortPlayerController::ServerReadyToStartMatch bSetupWorld = {}", bSetupWorld).c_str());
		}

		return ServerReadyToStartMatchOG(PC);
	}

	void (*ServerLoadingScreenDroppedOG)(AFortPlayerControllerAthena* PC);
	void ServerLoadingScreenDropped(AFortPlayerControllerAthena* PC)
	{
		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto GameState = (AFortGameStateAthena*)GameMode->GameState;

		AFortPlayerState* PS = (AFortPlayerState*)PC->PlayerState;

		return ServerLoadingScreenDroppedOG(PC);
	}

	void (*ServerExecuteInventoryItemOG)(AFortPlayerController* This, const FGuid& ItemGuid);
	void ServerExecuteInventoryItem(AFortPlayerController* This, const FGuid& ItemGuid)
	{
		AFortPlayerPawnAthena* Pawn = (AFortPlayerPawnAthena*)This->Pawn;

		FFortItemEntry* ItemEntry = FortInventory::FindItemEntry(This, ItemGuid);
		if (Pawn && ItemEntry) {
			Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)ItemEntry->ItemDefinition, ItemEntry->ItemGuid);
		}

		return ServerExecuteInventoryItemOG(This, ItemGuid);
	}

	void ServerCheat(AFortPlayerControllerAthena* PC, FString& Msg) {
		if (Globals::bIsProdServer)
			return;

		auto Gamemode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		std::string Command = Msg.ToString();
		Log(Command);

		if (Command == "SpawnBot") 
		{
			Bots::SpawnPlayerBot(PC->Pawn, PlayerBots::EBotState::Landed);
		}

		if (Command == "StartAircraftEarly")
		{
			UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"startaircraft", PC); //i think???
		}

		if (Command == "StartEvent") {
			UObject* TheEnd = nullptr;

			UClass* Scripting = StaticLoadObject<UClass>("/Game/Athena/Prototype/Blueprints/NightNight/BP_NightNight_Scripting.BP_NightNight_Scripting_C");

			TArray<AActor*> Scripts;
			UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), Scripting, &Scripts);

			if (Scripts.Num() > 0)
			{
				Log("can we not crash pls???");
				TheEnd = Scripts[0];
			}

			auto LoadLevel = StaticLoadObject<UFunction>("/Game/Athena/Prototype/Blueprints/NightNight/BP_NightNight_Scripting.BP_NightNight_Scripting_C.LoadNightNightLevel");
			TheEnd->ProcessEvent(LoadLevel, []() { static bool b = true; return &b; }());

			UFunction* StartEventFunc = TheEnd->Class->GetFunction("BP_NightNight_Scripting_C", "startevent");
			TheEnd->ProcessEvent(StartEventFunc, []() { static float f = 0.f; return &f; }());
		}
	}

	void ServerAttemptInventoryDrop(AFortPlayerController* PC, const FGuid& ItemGuid, int32 Count)
	{
		if (!PC) return;

		UFortItemDefinition* ItemDef = FortInventory::FindItemDefinition(PC, ItemGuid);
		if (!ItemDef) return;

		FFortItemEntry* ItemEntry = FortInventory::FindItemEntry(PC, ItemDef);
		if (!ItemEntry || ItemEntry->Count < Count) return;

		AFortPlayerPawn* PlayerPawn = PC->MyFortPawn;
		if (!PlayerPawn) return;

		FVector Drop = PlayerPawn->K2_GetActorLocation() + PlayerPawn->GetActorForwardVector() * 100.f;

		SpawnPickup(ItemDef, ItemEntry->Count, ItemEntry->LoadedAmmo, Drop, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset, PlayerPawn);
		FortInventory::RemoveItem(PC, ItemDef, Count);
		FortInventory::Update(PC, ItemEntry);
	}

	void (*ServerCreateBuildingActorOG)(AFortPlayerControllerAthena* PC, FCreateBuildingActorData& CreateBuildingData);
	void ServerCreateBuildingActor(AFortPlayerControllerAthena* PC, FCreateBuildingActorData& CreateBuildingData) {
		if (!PC) {
			Log("No PC!");
			return;
		}

		UClass* BuildingClass = PC->BroadcastRemoteClientInfo->RemoteBuildableClass.Get();

		TArray<ABuildingSMActor*> BuildingsToRemove;
		char BuildRestrictionFlag;
		if (CantBuild(UWorld::GetWorld(), BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, CreateBuildingData.bMirrored, &BuildingsToRemove, &BuildRestrictionFlag))
		{
			Log("CantBuild!");
			return;
		}

		auto ResourceItemDefinition = UFortKismetLibrary::GetDefaultObj()->K2_GetResourceItemDefinition(((ABuildingSMActor*)BuildingClass->DefaultObject)->ResourceType);
		FortInventory::RemoveItem(PC, ResourceItemDefinition, 10);

		ABuildingSMActor* PlacedBuilding = SpawnActor<ABuildingSMActor>(CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, PC, BuildingClass);
		PlacedBuilding->bPlayerPlaced = true;
		PlacedBuilding->InitializeKismetSpawnedBuildingActor(PlacedBuilding, PC, true);
		PlacedBuilding->TeamIndex = ((AFortPlayerStateAthena*)PC->PlayerState)->TeamIndex;
		PlacedBuilding->Team = EFortTeam(PlacedBuilding->TeamIndex);

		for (size_t i = 0; i < BuildingsToRemove.Num(); i++)
		{
			BuildingsToRemove[i]->K2_DestroyActor();
		}
		BuildingsToRemove.Free();
	}

	void ServerBeginEditingBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToEdit)
	{
		if (!BuildingActorToEdit || !BuildingActorToEdit->bPlayerPlaced || !PC->MyFortPawn)
			return;

		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		BuildingActorToEdit->SetNetDormancy(ENetDormancy::DORM_Awake);
		BuildingActorToEdit->EditingPlayer = PlayerState;

		for (int i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
		{
			auto Item = PC->WorldInventory->Inventory.ItemInstances[i];
			if (Item->GetItemDefinitionBP()->IsA(UFortEditToolItemDefinition::StaticClass()))
			{
				PC->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Item->GetItemDefinitionBP(), Item->GetItemGuid());
				break;
			}
		}

		if (!PC->MyFortPawn->CurrentWeapon || !PC->MyFortPawn->CurrentWeapon->IsA(AFortWeap_EditingTool::StaticClass()))
			return;

		AFortWeap_EditingTool* EditTool = (AFortWeap_EditingTool*)PC->MyFortPawn->CurrentWeapon;
		EditTool->EditActor = BuildingActorToEdit;
		EditTool->OnRep_EditActor();
	}
	 
	void ServerEndEditingBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToStopEditing) {
		if (!BuildingActorToStopEditing || !PC->MyFortPawn || BuildingActorToStopEditing->bDestroyed || BuildingActorToStopEditing->EditingPlayer != PC->PlayerState)
			return;
		BuildingActorToStopEditing->SetNetDormancy(ENetDormancy::DORM_DormantAll);
		BuildingActorToStopEditing->EditingPlayer = nullptr;
		for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
		{
			auto Item = PC->WorldInventory->Inventory.ItemInstances[i];
			if (Item->GetItemDefinitionBP()->IsA(UFortEditToolItemDefinition::StaticClass()))
			{
				PC->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Item->GetItemDefinitionBP(), Item->GetItemGuid());
				break;
			}
		}
		if (!PC->MyFortPawn->CurrentWeapon || !PC->MyFortPawn->CurrentWeapon->WeaponData || !PC->MyFortPawn->CurrentWeapon->IsA(AFortWeap_EditingTool::StaticClass()))
			return;

		AFortWeap_EditingTool* EditTool = (AFortWeap_EditingTool*)PC->MyFortPawn->CurrentWeapon;
		EditTool->EditActor = nullptr;
		EditTool->OnRep_EditActor();
	}

	void ServerEditBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored) {
		if (!BuildingActorToEdit || BuildingActorToEdit->EditingPlayer != PC->PlayerState || !NewBuildingClass.Get() || BuildingActorToEdit->bDestroyed)
			return;

		BuildingActorToEdit->SetNetDormancy(ENetDormancy::DORM_DormantAll);
		BuildingActorToEdit->EditingPlayer = nullptr;
		ABuildingSMActor* EditedBuildingActor = ReplaceBuildingActor(BuildingActorToEdit, 1, NewBuildingClass.Get(), 0, RotationIterations, bMirrored, PC);
		if (EditedBuildingActor)
			EditedBuildingActor->bPlayerPlaced = true;
	}

	void ServerRepairBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToRepair) {
		auto FortKismet = (UFortKismetLibrary*)UFortKismetLibrary::StaticClass()->DefaultObject;
		if (!BuildingActorToRepair)
			return;

		if (BuildingActorToRepair->EditingPlayer)
		{
			return;
		}

		float BuildingHealthPercent = BuildingActorToRepair->GetHealthPercent();
		float BuildingCost = 10;
		float RepairCostMultiplier = 0.75;

		float BuildingHealthPercentLost = 1.0f - BuildingHealthPercent;
		float RepairCostUnrounded = (BuildingCost * BuildingHealthPercentLost) * RepairCostMultiplier;
		float RepairCost = std::floor(RepairCostUnrounded > 0 ? RepairCostUnrounded < 1 ? 1 : RepairCostUnrounded : 0);
		if (RepairCost < 0)
			return;

		auto ResourceDef = FortKismet->K2_GetResourceItemDefinition(BuildingActorToRepair->ResourceType);
		if (!ResourceDef)
			return;

		if (!PC->bBuildFree)
		{
			FortInventory::RemoveItem(PC, ResourceDef, (int)RepairCost);
		}

		BuildingActorToRepair->RepairBuilding(PC, (int)RepairCost);
	}

	void (*ClientOnPawnDiedOG)(AFortPlayerControllerZone* PC, const FFortPlayerDeathReport& DeathReport);
	void ClientOnPawnDied(AFortPlayerControllerAthena* PC, const FFortPlayerDeathReport& DeathReport)
	{
		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		if (!PlayerState) {
			return;
		}
		FVector DeathLocation = PC->Pawn->K2_GetActorLocation();
		Log(PlayerState->GetPlayerName().ToString() + " Died!");

		if (!GameState->IsRespawningAllowed(PlayerState))
		{
			if (PC && PC->WorldInventory)
			{
				for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					if (((UFortWorldItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->bCanBeDropped)
					{
						SpawnPickup(PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.ItemDefinition, PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.Count, PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.LoadedAmmo, DeathLocation, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination, PC->MyFortPawn);
					}
				}
			}

			FAthenaRewardResult Result;
			PC->ClientSendEndBattleRoyaleMatchForPlayer(true, Result);

			FAthenaMatchStats Stats;
			FAthenaMatchTeamStats TeamStats;

			if (PlayerState)
			{
				PlayerState->Place = GameMode->AliveBots.Num() + GameMode->AlivePlayers.Num();
				PlayerState->OnRep_Place();
			}

			for (size_t i = 0; i < 20; i++)
			{
				Stats.Stats[i] = 0;
			}

			Stats.Stats[3] = PlayerState->KillScore;

			TeamStats.Place = PlayerState->Place;
			TeamStats.TotalPlayers = GameState->TotalPlayers;

			PC->ClientSendMatchStatsForPlayer(Stats);
			PC->ClientSendTeamStatsForPlayer(TeamStats);
		}

		return ClientOnPawnDiedOG(PC, DeathReport);
	}

	void ServerSetInAircraft(AFortPlayerStateAthena* PlayerState, bool bNewInAircraft)
	{
		Log("ServerSetInAircraft Called!");
		if (!PlayerState) return;

		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)PlayerState->GetOwner();
		if (!PC) return;

		for (int i = PC->WorldInventory->Inventory.ReplicatedEntries.Num() - 1; i >= 0;i--)
		{
			FFortItemEntry* ItemEntry = &PC->WorldInventory->Inventory.ReplicatedEntries[i];
			if (!ItemEntry) continue;
			if (!((UFortWorldItemDefinition*)ItemEntry->ItemDefinition)->bCanBeDropped) continue;
			FortInventory::RemoveItem(PC, ItemEntry->ItemDefinition, ItemEntry->Count);
		}
	}

	void (*ServerAttemptInteractOG)(UFortControllerComponent_Interaction* ComponentInteraction, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalObjectData);
	void ServerAttemptInteract(UFortControllerComponent_Interaction* ComponentInteraction, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalObjectData)
	{
		if (!ComponentInteraction || !ReceivingActor) return ServerAttemptInteractOG(ComponentInteraction, ReceivingActor, InteractComponent, InteractType, OptionalObjectData);
		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)ComponentInteraction->GetOwner();
		if (!PC || !PC->MyFortPawn) return ServerAttemptInteractOG(ComponentInteraction, ReceivingActor, InteractComponent, InteractType, OptionalObjectData);

		Log("ServerAttemptInteract: " + ReceivingActor->Class->GetFullName());

		if (ReceivingActor->Class->GetFullName().contains("AthenaSupplyDrop"))
		{
			std::vector<FFortItemEntry> LootDrops = Looting::PickLootDrops(UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaSupplyDrop"));

			for (const FFortItemEntry& ItemEntry : LootDrops)
			{
				SpawnPickup(ItemEntry, ReceivingActor->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::SupplyDrop);
			}
		}

		return ServerAttemptInteractOG(ComponentInteraction, ReceivingActor, InteractComponent, InteractType, OptionalObjectData);
	}

	void ServerPlayEmoteItem(AFortPlayerControllerAthena* PC, UFortMontageItemDefinitionBase* EmoteAsset, float EmoteRandomNumber) {
		Log("ServerPlayEmoteItem Called!");

		if (!PC || !EmoteAsset)
			return;

		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		if (GameState->GamePhase == EAthenaGamePhase::Aircraft) {
			return;
		}

		UClass* DanceAbility = StaticLoadObject<UClass>("/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");
		UClass* SprayAbility = StaticLoadObject<UClass>("/Game/Abilities/Sprays/GAB_Spray_Generic.GAB_Spray_Generic_C");

		if (!DanceAbility || !SprayAbility)
			return;

		auto EmoteDef = (UAthenaDanceItemDefinition*)EmoteAsset;
		if (!EmoteDef)
			return;

		PC->MyFortPawn->bMovingEmote = EmoteDef->bMovingEmote;
		PC->MyFortPawn->EmoteWalkSpeed = EmoteDef->WalkForwardSpeed;
		PC->MyFortPawn->bMovingEmoteForwardOnly = EmoteDef->bMoveForwardOnly;

		FGameplayAbilitySpec Spec{};
		UGameplayAbility* Ability = nullptr;

		if (EmoteAsset->IsA(UAthenaSprayItemDefinition::StaticClass()))
		{
			Ability = (UGameplayAbility*)SprayAbility->DefaultObject;
		}

		if (Ability == nullptr) {
			Ability = (UGameplayAbility*)DanceAbility->DefaultObject;
		}

		AbilitySpecConstructor(&Spec, Ability, 1, -1, EmoteDef);
		GiveAbilityAndActivateOnce(((AFortPlayerStateAthena*)PC->PlayerState)->AbilitySystemComponent, &Spec.Handle, Spec);
	}

	void ServerReturnToMainMenu(AFortPlayerControllerAthena* PC)
	{
		PC->ClientReturnToMainMenu(L"");
	}

	void HookAll()
	{
		HookVTable<AFortPlayerControllerAthena>(0x108, ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);
		
		MH_CreateHook((LPVOID)(ImageBase + 0x19C7940), ServerReadyToStartMatch, (LPVOID*)&ServerReadyToStartMatchOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x1F19990), ServerLoadingScreenDropped, (LPVOID*)&ServerLoadingScreenDroppedOG);

		HookVTable<AFortPlayerControllerAthena>(0x1FE, ServerExecuteInventoryItem, (LPVOID*)&ServerExecuteInventoryItemOG);

		HookVTable<AFortPlayerControllerAthena>(0x1BC, ServerCheat);
		HookVTable<AFortPlayerControllerAthena>(0x210, ServerAttemptInventoryDrop);
		HookVTable<AFortPlayerControllerAthena>(0x224, ServerCreateBuildingActor);
		HookVTable<AFortPlayerControllerAthena>(0x22A, ServerBeginEditingBuildingActor);
		HookVTable<AFortPlayerControllerAthena>(0x228, ServerEndEditingBuildingActor);
		HookVTable<AFortPlayerControllerAthena>(0x226, ServerEditBuildingActor);
		HookVTable<AFortPlayerControllerAthena>(0x220, ServerRepairBuildingActor);

		MH_CreateHook((LPVOID)(ImageBase + 0x1F34E50), ClientOnPawnDied, (LPVOID*)&ClientOnPawnDiedOG);

		HookVTable<AFortPlayerStateAthena>(0x103, ServerSetInAircraft);
		HookVTable<UFortControllerComponent_Interaction>(0x81, ServerAttemptInteract, (LPVOID*)&ServerAttemptInteractOG);

		HookVTable<AFortPlayerControllerAthena>(0x1BE, ServerPlayEmoteItem);
		HookVTable<AFortPlayerControllerAthena>(0x258, ServerReturnToMainMenu);

		Log("Controller Hooked!");
	}
}