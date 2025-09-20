#pragma once
#include "framework.h"
#include "FortInventory.h"
#include "AbilitySystemComponent.h"
#include "Looting.h"
#include "Vehicles.h"
#include "FortAIBotControllerAthena.h"
#include "GameMode.h"

namespace Controller
{
	void (*ServerAcknowledgePossessionOG)(APlayerController* PC, APawn* P);
	void ServerAcknowledgePossession(APlayerController* PC, APawn* P)
	{
		PC->AcknowledgedPawn = P;
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
	}

	void (*ServerLoadingScreenDroppedOG)(AFortPlayerControllerAthena* PC);
	void ServerLoadingScreenDropped(AFortPlayerControllerAthena* PC)
	{
		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto GameState = (AFortGameStateAthena*)GameMode->GameState;

		AFortPlayerState* PS = (AFortPlayerState*)PC->PlayerState;

		return ServerLoadingScreenDroppedOG(PC);
	}

	void ServerExecuteInventoryItem(AFortPlayerController* PC, const FGuid& ItemGuid)
	{
		if (!PC || !PC->MyFortPawn)
			return;

		if (PC->IsInAircraft())
			return;

		UFortWeaponItemDefinition* ItemDefinition = (UFortWeaponItemDefinition*)FortInventory::FindItemDefinition(PC, ItemGuid);
		PC->MyFortPawn->EquipWeaponDefinition(ItemDefinition, ItemGuid);
	}

	void OnDamageServer(ABuildingActor* BuildingActor, float Damage, const struct FGameplayTagContainer& DamageTags, const struct FVector& Momentum, const struct FHitResult& HitInfo, AController* InstigatedBy, AActor* DamageCauser, const struct FGameplayEffectContextHandle& EffectContext)
	{
		if (!BuildingActor || !InstigatedBy) return;
		if (BuildingActor->IsA(ABuildingSMActor::StaticClass()) && InstigatedBy->IsA(AAthena_PlayerController_C::StaticClass()))
		{
			ABuildingSMActor* BuildingSMActor = (ABuildingSMActor*)BuildingActor;
			AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)InstigatedBy;
			if (!PC->MyFortPawn) return;
			if (BuildingSMActor->bDestroyed || BuildingSMActor->bPlayerPlaced) return;

			AFortWeapon* Weapon = (AFortWeapon*)PC->MyFortPawn->CurrentWeapon;
			if (!Weapon) return;
			if (Weapon->WeaponData->IsA(UFortWeaponRangedItemDefinition::StaticClass())) return;
			UFortWeaponMeleeItemDefinition* PickaxeItemDefinition = (UFortWeaponMeleeItemDefinition*)Weapon->WeaponData;
			if (!PickaxeItemDefinition) return;

			int DamageCount = (Damage == 100.f) ? UKismetMathLibrary::RandomFloatInRange(15, 20) : UKismetMathLibrary::RandomFloatInRange(5, 10);

			PC->ClientReportDamagedResourceBuilding(BuildingSMActor, BuildingSMActor->ResourceType, DamageCount, BuildingSMActor->bDestroyed, Damage == 100.f);

			UFortResourceItemDefinition* ResourceItemDefinition = UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingSMActor->ResourceType);
			if (!ResourceItemDefinition) return;


			FFortItemEntry* ItemEntry = FortInventory::FindItemEntry(PC, ResourceItemDefinition);

			if (!ItemEntry) // if item don't exist we give them the newest item ever!
			{
				//Log("Holy Give!");
				FortInventory::GiveItem(PC, ResourceItemDefinition, DamageCount);
			}
			else
			{
				int MaxStackSize = ItemEntry->ItemDefinition->MaxStackSize;
				int Count = ItemEntry->Count;

				if (Count >= MaxStackSize)
				{
					//Log("SpawnPickup Full!");
					SpawnPickup(ItemEntry->ItemDefinition, DamageCount, ItemEntry->LoadedAmmo, PC->MyFortPawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PC->MyFortPawn);
				}
				else
				{
					//Log("Overflow Stack!");
					int Space = MaxStackSize - Count;
					int AddToStack = UKismetMathLibrary::min_0(Space, DamageCount);
					int Overflow = DamageCount - AddToStack;

					ItemEntry->Count += AddToStack;
					PC->WorldInventory->Inventory.MarkItemDirty(*ItemEntry);

					if (Overflow > 0) SpawnPickup(ItemEntry->ItemDefinition, Overflow, ItemEntry->LoadedAmmo, PC->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PC->MyFortPawn);
				}
			}

			FortInventory::Update(PC);
		}
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
			FortAIBotControllerAthena::SpawnPlayerBot(PC->Pawn, PlayerBots::EBotState::Landed);
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

	void ServerCreateBuildingActor(AFortPlayerControllerAthena* PC, const FCreateBuildingActorData& CreateBuildingData)
	{
		if (!PC || PC->IsInAircraft()) return;

		AFortBroadcastRemoteClientInfo* ClientInfo = PC->BroadcastRemoteClientInfo;

		UClass* BuildingClass = ClientInfo->RemoteBuildableClass.Get();

		char Idk;
		TArray<AActor*> BuildingActors;

		bool bCantBuild = !CantBuild(UWorld::GetWorld(), BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, CreateBuildingData.bMirrored, &BuildingActors, &Idk);

		if (bCantBuild)
		{
			UFortItemDefinition* ResourceDef = UFortKismetLibrary::K2_GetResourceItemDefinition(ClientInfo->RemoteBuildingMaterial);
			if (ResourceDef)
				FortInventory::RemoveItem(PC, ResourceDef, 10);

			ABuildingSMActor* NewBuilding = SpawnActor<ABuildingSMActor>(CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, PC, BuildingClass);
			if (!NewBuilding) return;

			NewBuilding->bPlayerPlaced = true;
			NewBuilding->InitializeKismetSpawnedBuildingActor(NewBuilding, PC, true);
			
			if (AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState)
			{
				NewBuilding->TeamIndex = PlayerState->TeamIndex;
				NewBuilding->Team = EFortTeam(PlayerState->TeamIndex);
				NewBuilding->OnRep_Team();
			}

			for (int i = 0; i < BuildingActors.Num(); i++)
				BuildingActors[i]->K2_DestroyActor();

			BuildingActors.Free();
		}
	}

	void ServerBeginEditingBuildingActor(AFortPlayerController* PC, ABuildingSMActor* BuildingActorToEdit)
	{
		if (!PC || PC->IsInAircraft() || !PC->MyFortPawn || !BuildingActorToEdit) return;

		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		if (!PlayerState) return;

		FFortItemEntry* EditToolEntries = FortInventory::FindItemEntry(PC, EditTool);
		if (!EditToolEntries) return;

		ServerExecuteInventoryItem(PC, EditToolEntries->ItemGuid);

		BuildingActorToEdit->EditingPlayer = PlayerState;
		BuildingActorToEdit->OnRep_EditingPlayer();
		BuildingActorToEdit->SetNetDormancy(ENetDormancy::DORM_Awake); //i think it should be awake??

		AFortWeap_EditingTool* EditingTool = (AFortWeap_EditingTool*)PC->MyFortPawn->CurrentWeapon;
		//if (!EditingTool) return;

		EditingTool->EditActor = BuildingActorToEdit;
		EditingTool->OnRep_EditActor();
	}

	void ServerEndEditingBuildingActor(AFortPlayerController* PC, ABuildingSMActor* BuildingActorToStopEditing)
	{
		if (!PC || PC->IsInAircraft() || !PC->MyFortPawn || !BuildingActorToStopEditing || BuildingActorToStopEditing->EditingPlayer != PC->PlayerState) return;

		BuildingActorToStopEditing->EditingPlayer = nullptr;
		BuildingActorToStopEditing->OnRep_EditingPlayer();
		BuildingActorToStopEditing->SetNetDormancy(ENetDormancy::DORM_Initial); //Hopefully Initial works

		AFortWeap_EditingTool* EditingTool = (AFortWeap_EditingTool*)PC->MyFortPawn->CurrentWeapon;
		if (!EditingTool) return;

		EditingTool->EditActor = nullptr;
		EditingTool->OnRep_EditActor();
	}

	void ServerEditBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored)
	{
		if (!PC || PC->IsInAircraft() || !PC->MyFortPawn || !BuildingActorToEdit || BuildingActorToEdit->bDestroyed) return;

		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		if (!PlayerState || BuildingActorToEdit->EditingPlayer != PlayerState) return;

		BuildingActorToEdit->EditingPlayer = nullptr;
		BuildingActorToEdit->OnRep_EditingPlayer();
		BuildingActorToEdit->SetNetDormancy(ENetDormancy::DORM_Initial);

		ABuildingSMActor* NewBuilding = ReplaceBuildingActor(BuildingActorToEdit, 1, NewBuildingClass.Get(), 0, RotationIterations, bMirrored, PC);
		if (!NewBuilding) return;

		NewBuilding->bPlayerPlaced = true;
		NewBuilding->InitializeKismetSpawnedBuildingActor(NewBuilding, PC, true);

		NewBuilding->TeamIndex = PlayerState->TeamIndex;
		NewBuilding->Team = EFortTeam(PlayerState->TeamIndex);
		NewBuilding->OnRep_Team();
	}

	void (*ClientOnPawnDiedOG)(AFortPlayerControllerZone* PC, const FFortPlayerDeathReport& DeathReport);
	void ClientOnPawnDied(AFortPlayerControllerAthena* PC, const FFortPlayerDeathReport& DeathReport)
	{
		Log("ClientOnPawnDied Function Called!");

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)GameMode->GameState;
		if (!GameMode || !GameState) return ClientOnPawnDiedOG(PC, DeathReport);

		if (!PC || !PC->MyFortPawn) return ClientOnPawnDiedOG(PC, DeathReport);

		AFortPlayerStateAthena* DeadPlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		if (!DeadPlayerState) return ClientOnPawnDiedOG(PC, DeathReport);

		AFortPlayerPawnAthena* KillerPawn = (AFortPlayerPawnAthena*)DeathReport.KillerPawn;
		AFortPlayerStateAthena* KillerPlayerState = (AFortPlayerStateAthena*)DeathReport.KillerPlayerState;
		AFortPlayerController* KillerController = KillerPawn ? (AFortPlayerController*)KillerPawn->Controller : nullptr;

		bool bIdk = KillerPawn == PC->MyFortPawn;

	    FDeathInfo& DeathInfo = DeadPlayerState->DeathInfo;
		
		DeathInfo.bDBNO = PC->MyFortPawn->bWasDBNOOnDeath;
		DeathInfo.bInitialized = true;
		DeathInfo.DeathLocation = PC->Pawn->K2_GetActorLocation();
		DeathInfo.DeathTags = DeathReport.Tags;
		DeathInfo.Downer = KillerPlayerState;
		DeathInfo.Distance = (KillerPawn ? KillerPawn->GetDistanceTo(PC->Pawn) : ((AFortPlayerPawnAthena*)PC->Pawn)->LastFallDistance);
		DeadPlayerState->DeathInfo.FinisherOrDowner = DeathReport.KillerPlayerState ? DeathReport.KillerPlayerState : PC->PlayerState;
		DeathInfo.DeathCause = DeadPlayerState->ToDeathCause(DeathInfo.DeathTags, DeathInfo.bDBNO);
		DeadPlayerState->OnRep_DeathInfo();

		PC->MyFortPawn->ForceNetUpdate();

		for (int i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			FFortItemEntry* ItemEntry = &PC->WorldInventory->Inventory.ReplicatedEntries[i];
			if (!ItemEntry) continue;

			if (FortInventory::GetQuickBars(ItemEntry->ItemDefinition) == EFortQuickBars::Primary ||
				ItemEntry->ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) || 
				ItemEntry->ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || 
				ItemEntry->ItemDefinition->IsA(UFortTrapItemDefinition::StaticClass()))
			{
				FVector Drop = PC->MyFortPawn->K2_GetActorLocation() + PC->MyFortPawn->GetActorForwardVector() * 100.0f;
				SpawnPickup(ItemEntry->ItemDefinition, ItemEntry->Count, ItemEntry->LoadedAmmo, Drop, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::PlayerElimination, PC->MyFortPawn);
			}
		}

		PC->WorldInventory->Inventory.ReplicatedEntries.Free();
		FortInventory::Update(PC);

		RemoveFromAlivePlayers(GameMode, PC, DeadPlayerState, PC->MyFortPawn, nullptr, (uint8)DeathInfo.DeathCause, 0);

		if (!bIdk && KillerPlayerState) //opps???
		{
			KillerPlayerState->KillScore++;
			KillerPlayerState->OnRep_Kills();
		}

		//Log(std::format("PlayerName={} Killed PlayerName={}", KillerPlayerState->GetPlayerName().ToString(), DeadPlayerState->GetPlayerName().ToString()).c_str());

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

	void HookAll()
	{
		HookVTable<AFortPlayerControllerAthena>(0x108, ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);
		
		MH_CreateHook((LPVOID)(ImageBase + 0x19C7940), ServerReadyToStartMatch, (LPVOID*)&ServerReadyToStartMatchOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x1F19990), ServerLoadingScreenDropped, (LPVOID*)&ServerLoadingScreenDroppedOG);

		HookVTable<AFortPlayerControllerAthena>(0x1FE, ServerExecuteInventoryItem);

		MH_CreateHook((LPVOID)(ImageBase + 0x1CC36A0), OnDamageServer, nullptr);

		HookVTable<AFortPlayerControllerAthena>(0x1BC, ServerCheat);
		HookVTable<AFortPlayerControllerAthena>(0x210, ServerAttemptInventoryDrop);
		HookVTable<AFortPlayerControllerAthena>(0x224, ServerCreateBuildingActor);
		HookVTable<AFortPlayerControllerAthena>(0x22A, ServerBeginEditingBuildingActor);
		HookVTable<AFortPlayerControllerAthena>(0x228, ServerEndEditingBuildingActor);

		MH_CreateHook((LPVOID)(ImageBase + 0x1F34E50), ClientOnPawnDied, (LPVOID*)&ClientOnPawnDiedOG);

		HookVTable<AFortPlayerStateAthena>(0x103, ServerSetInAircraft);
		HookVTable<UFortControllerComponent_Interaction>(0x80, ServerAttemptInteract, (LPVOID*)&ServerAttemptInteractOG);

		Log("Controller Hooked!");
	}
}