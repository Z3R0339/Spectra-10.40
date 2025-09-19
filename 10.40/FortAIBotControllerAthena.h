#pragma once
#include "framework.h"
#include "PlayerBots.h"
#include "BotNames.h"

namespace FortAIBotControllerAthena
{
	static std::vector<UAthenaCharacterItemDefinition*> Characters{};
	static std::vector<UAthenaPickaxeItemDefinition*> Pickaxes{};
	static std::vector<UAthenaBackpackItemDefinition*> Backpacks{};
	static std::vector<UAthenaGliderItemDefinition*> Gliders{};
	static std::vector<UAthenaSkyDiveContrailItemDefinition*> Contrails{};
	inline std::vector<UAthenaDanceItemDefinition*> Dances{};

	void SpawnPlayerBot(AActor* OverrideSpawnLoc = nullptr, PlayerBots::EBotState StartingState = PlayerBots::EBotState::Warmup) {
		static UBehaviorTree* PhoebeBehaviorTree = StaticLoadObject<UBehaviorTree>("Game/Athena/AI/Phoebe/BehaviorTrees/BT_Phoebe.BT_Phoebe");
		static UClass* PhoebePawnClass = StaticLoadObject<UClass>("/Game/Athena/AI/Phoebe/BP_PlayerPawn_Athena_Phoebe.BP_PlayerPawn_Athena_Phoebe_C");

		static int MaxSpawnLocations = PlayerStarts.Num();
		static int BotSpawnLocationIndex = 0;

		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		UFortAthenaAIBotCustomizationData* BotCustomizationData = (UFortAthenaAIBotCustomizationData*)UGameplayStatics::SpawnObject(UFortAthenaAIBotCustomizationData::StaticClass(), GameMode);
		BotCustomizationData->BehaviorTree = PhoebeBehaviorTree;
		BotCustomizationData->bOverrideBehaviorTree = true;
		BotCustomizationData->bOverrideCharacterCustomization = false;
		BotCustomizationData->bOverrideDBNOPlayStyle = false;
		BotCustomizationData->bOverrideSkillLevel = false;
		BotCustomizationData->bOverrideSkillSets = false;
		BotCustomizationData->bOverrideStartupInventory = false;
		BotCustomizationData->CharacterCustomization = {};
		BotCustomizationData->DBNOPlayStyle = EDBNOPlayStyle::Default;
		BotCustomizationData->PawnClass = PhoebePawnClass;
		BotCustomizationData->SkillLevel = 1;
		BotCustomizationData->SkillSets = {};
		BotCustomizationData->StartupInventory = (UFortAthenaAIBotInventoryItems*)UGameplayStatics::SpawnObject(UFortAthenaAIBotInventoryItems::StaticClass(), GameMode);

		static UFortWeaponMeleeItemDefinition* PickDef = StaticLoadObject<UFortWeaponMeleeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");;
		if (!Pickaxes.empty()) {
			PickDef = Pickaxes[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Pickaxes.size() - 1)]->WeaponDefinition;
		}

		if (PickDef) {
			BotCustomizationData->StartupInventory->Items.Add(PickDef);
		}

		if (Floor && Roof && Stair && Wall && EditTool) {
			BotCustomizationData->StartupInventory->Items.Add(Floor);
			BotCustomizationData->StartupInventory->Items.Add(Roof);
			BotCustomizationData->StartupInventory->Items.Add(Stair);
			BotCustomizationData->StartupInventory->Items.Add(Wall);
			BotCustomizationData->StartupInventory->Items.Add(EditTool);
		}
		else 
		{
			Log("BuildingItem Null!");
		}

		//AActor* BotSpawn = PlayerStarts[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, PlayerStarts.Num() - 1)];
		if (BotSpawnLocationIndex >= MaxSpawnLocations) {
			BotSpawnLocationIndex = 0;
		}
		AActor* BotSpawn = PlayerStarts[BotSpawnLocationIndex];
		BotSpawnLocationIndex++;
		if (OverrideSpawnLoc) {
			BotSpawn = OverrideSpawnLoc;
		}
		AFortPlayerPawnAthena* Pawn = GameMode->ServerBotManager->SpawnBot(BotSpawn->K2_GetActorLocation(), BotSpawn->K2_GetActorRotation(), BotCustomizationData);
		if (!Pawn)
			return;
		AFortAthenaAIBotController* PC = (AFortAthenaAIBotController*)Pawn->Controller;
		if (!PC)
			return;
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		if (!PlayerState)
			return;

		if (BotDisplayNames.size() != 0) {
			std::srand(static_cast<unsigned int>(std::time(0)));
			int randomIndex = std::rand() % BotDisplayNames.size();
			std::string rdName = BotDisplayNames[randomIndex];
			BotDisplayNames.erase(BotDisplayNames.begin() + randomIndex);

			int size_needed = MultiByteToWideChar(CP_UTF8, 0, rdName.c_str(), (int)rdName.size(), NULL, 0);
			std::wstring wideString(size_needed, 0);
			MultiByteToWideChar(CP_UTF8, 0, rdName.c_str(), (int)rdName.size(), &wideString[0], size_needed);

			FString CVName = FString(wideString.c_str());
			GameMode->ChangeName(PC, CVName, true);
			PlayerState->OnRep_PlayerName();
		}

		if (StartingState != PlayerBots::EBotState::Warmup) {
			for (PlayerBots::PlayerBot* Bot : PlayerBots::PlayerBotArray) {
				if (Bot->Pawn == Pawn) {
					Bot->BotState = StartingState;
				}
			}
		}

		if (true)
		{
			int Ret = Globals::NextBotTeamIndex;

			++Globals::CurrentBotsOnTeam;

			if (Globals::CurrentBotsOnTeam == Globals::MaxPlayersPerTeam)
			{
				Globals::NextBotTeamIndex++;
				Globals::CurrentBotsOnTeam = 0;
			}

			PlayerState->TeamIndex = Ret;
			PlayerState->OnRep_TeamIndex(0); //i think??

			PlayerState->SquadId = PlayerState->TeamIndex - 3;
			PlayerState->OnRep_SquadId();

			PlayerState->OnRep_PlayerTeam();
			PlayerState->OnRep_PlayerTeamPrivate();

			FGameMemberInfo NewMemberInfo{ -1,-1,-1 };
			NewMemberInfo.MemberUniqueId = PlayerState->GetUniqueID();
			NewMemberInfo.SquadId = PlayerState->SquadId;
			NewMemberInfo.TeamIndex = PlayerState->TeamIndex;

			GameState->GameMemberInfoArray.Members.Add(NewMemberInfo);
			GameState->GameMemberInfoArray.MarkItemDirty(NewMemberInfo);
		}

		//Log("TeamIndex: " + std::to_string(PlayerState->TeamIndex));
	}

	AFortPlayerPawnAthena* (*SpawnBotOG)(UFortServerBotManagerAthena* This, FVector SpawnLoc, FRotator SpawnRot, UFortAthenaAIBotCustomizationData* BotData);
	AFortPlayerPawnAthena* SpawnBot(UFortServerBotManagerAthena* This, FVector SpawnLoc, FRotator SpawnRot, UFortAthenaAIBotCustomizationData* BotData)
	{
		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		//Log("SpawnBot Called!");
		AFortPlayerPawnAthena* Pawn = SpawnBotOG(This, SpawnLoc, SpawnRot, BotData);
		AFortAthenaAIBotController* PC = (AFortAthenaAIBotController*)Pawn->Controller;
		if (!PC) {
			Log("UFortServerBotManagerAthena::SpawnBot, Spawning New Controller!");
			PC = SpawnActor<AFortAthenaAIBotController>({});
			PC->Possess(Pawn);
		}
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		if (!PC || !Pawn) {
			Log("UFortServerBotManagerAthena::SpawnBot Failed! Unable to spawn pawn or get controller!");
			return Pawn;
		}

		if (!Characters.empty()) {
			auto CID = Characters[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Characters.size() - 1)];
			if (CID->HeroDefinition)
			{
				if (CID->HeroDefinition->Specializations.IsValid())
				{
					for (size_t i = 0; i < CID->HeroDefinition->Specializations.Num(); i++)
					{
						UFortHeroSpecialization* Spec = StaticLoadObject<UFortHeroSpecialization>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(CID->HeroDefinition->Specializations[i].ObjectID.AssetPathName).ToString());
						if (Spec)
						{
							for (size_t j = 0; j < Spec->CharacterParts.Num(); j++)
							{
								UCustomCharacterPart* Part = StaticLoadObject<UCustomCharacterPart>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(Spec->CharacterParts[j].ObjectID.AssetPathName).ToString());
								if (Part)
								{
									PlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
								}
							}
						}
					}
				}
			}
			if (CID) {
				Pawn->CosmeticLoadout.Character = CID;
			}
		}
		if (!Backpacks.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.5)) { // less likely to equip than skin cause lots of ppl prefer not to use backpack
			auto Backpack = Backpacks[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Backpacks.size() - 1)];
			for (size_t j = 0; j < Backpack->CharacterParts.Num(); j++)
			{
				UCustomCharacterPart* Part = Backpack->CharacterParts[j];
				if (Part)
				{
					PlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
				}
			}
		}
		if (!Gliders.empty()) {
			auto Glider = Gliders[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Gliders.size() - 1)];
			Pawn->CosmeticLoadout.Glider = Glider;
		}
		if (!Contrails.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.95)) {
			auto Contrail = Contrails[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Contrails.size() - 1)];
			Pawn->CosmeticLoadout.SkyDiveContrail = Contrail;
		}
		for (size_t i = 0; i < Dances.size(); i++)
		{
			Pawn->CosmeticLoadout.Dances.Add(Dances.at(i));
		}

		Pawn->OnRep_CosmeticLoadout();
		PlayerState->OnRep_CharacterData();

		PC->BehaviorTree = BotData->BehaviorTree;
		PC->RunBehaviorTree(BotData->BehaviorTree);

		Pawn->SetMaxHealth(100.f);
		Pawn->SetMaxShield(100.f);
		Pawn->SetHealth(100.f);
		Pawn->SetShield(0.f);

		if (!PC->Inventory) {
			// Until i hook SetUpInventoryBot
			//Log("Creating inv for bot!");
			PC->Inventory = SpawnActor<AFortInventory>({});
			PC->Inventory->Owner = PC;
			PC->Inventory->OnRep_Owner();
		}

		PlayerBots::PlayerBot* Bot = new PlayerBots::PlayerBot(Pawn, PC, PlayerState);

		if (PC->Inventory) {
			for (auto item : BotData->StartupInventory->Items)
			{
				if (item)
				{
					Bot->GiveItem(item, 1, 0);
				}
			}
		}
		else {
			Log("Bruh!!!");
		}

		Bot->EquipPickaxe();

		return Bot->Pawn;
	}

	inline char (*SetUpInventoryBotOG)(AActor* Pawn, __int64 a2);
	inline char SetUpInventoryBot(AActor* Pawn, __int64 a2)
	{
		Log("SetUpInventoryBot Called!");
		((AFortAthenaAIBotController*)((AFortPlayerPawnAthena*)Pawn)->Controller)->Inventory = SpawnActor<AFortInventory>({}, {}, Pawn);

		return SetUpInventoryBotOG(Pawn, a2);
	}

	__int64 (*GatherNearestLootOG)(UFortAthenaAIBotEvaluator* a1);
	__int64 __fastcall GatherNearestLoot(UFortAthenaAIBotEvaluator* a1)
	{
		//Tim2025: add anything in here...
		Log("GatherNearestLoot Called!");
		return GatherNearestLootOG(a1);
	}

	//Tim2025: i will look for this in the future
	__int64 (*InitializeForWorldOG)(UAthenaNavSystem* a1, __int64 a2, unsigned __int8 a3);
	__int64 __fastcall InitializeForWorld(UAthenaNavSystem* a1, __int64 a2, unsigned __int8 a3)
	{
		Log("InitializeForWorld: " + a1->GetFullName());
		return InitializeForWorldOG(a1, a2, a3);
	}

	void (*OnActorBumpOG)(UPathFollowingComponent* Comp, AActor* SelfActor, AActor* OtherActor, const FVector& NormalImpulse, FHitResult& Hit);
	void OnActorBump(UFortAthenaAIBotPathFollowingComponent* Comp, AActor* SelfActor, AActor* OtherActor, const FVector& NormalImpulse, FHitResult& Hit)
	{
		for (PlayerBots::PlayerBot* bot : PlayerBots::PlayerBotArray)
		{
			if (bot->PC == Comp->BotController)
			{
				if (bot->BotState < PlayerBots::EBotState::Landed) {
					break;
				}

				if (bot->bPotentiallyUnderAttack) {
					if (!bot->Pawn->bIsCrouched && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.025f)) {
						bot->Pawn->Crouch(false);
					}

					if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.05f)) {
						bot->Pawn->UnCrouch(false);
						bot->Pawn->Jump();
					}
					break;
				}
				bot->bPauseTicking = true;
				if (!bot->IsPickaxeEquiped()) {
					bot->EquipPickaxe();
				}
				bot->Pawn->PawnStartFire(0);

				if (!bot->Pawn->bIsCrouched && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.025f)) {
					bot->Pawn->Crouch(false);
				}

				if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.05f)) {
					bot->Pawn->UnCrouch(false);
					bot->Pawn->Jump();
				}

				bot->ForceStrafe(true);

				bot->Pawn->PawnStopFire(0);
				bot->bPauseTicking = false;
				break;
			}
		}

		return OnActorBumpOG(Comp, SelfActor, OtherActor, NormalImpulse, Hit);
	}

	// Doesent exist in this version ig, ggs :fire!
	void (*OnPossessedPawnDiedOG)(AFortAthenaAIBotController* PC, AActor* DamagedActor, float Damage, AController* InstigatedBy, AActor* DamageCauser, FVector HitLocation, UPrimitiveComponent* HitComp, FName BoneName, FVector Momentum);
	void OnPossessedPawnDied(AFortAthenaAIBotController* PC, AActor* DamagedActor, float Damage, AController* InstigatedBy, AActor* DamageCauser, FVector HitLocation, UPrimitiveComponent* HitComp, FName BoneName, FVector Momentum)
	{
		if (!PC) {
			return;
		}

		if (InstigatedBy && DamageCauser) {
			for (PlayerBots::PlayerBot* bot : PlayerBots::PlayerBotArray)
			{
				if (bot && bot->PC && bot->PC == PC)
				{
					break;
				}
			}
		}

		for (int32 i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
				continue;
			auto Def = PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition;
			SpawnPickup(Def, 0, 0, PC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::PlayerElimination);
		}

		return OnPossessedPawnDiedOG(PC, DamagedActor, Damage, InstigatedBy, DamageCauser, HitLocation, HitComp, BoneName, Momentum);
	}

	void (*OnAgentDBNOOG)(AFortAthenaAIBotController* AI, AFortPawn* InPlayer, bool bInIsDBNO);
	void OnAgentDBNO(AFortAthenaAIBotController* AI, AFortPawn* InPlayer, bool bInIsDBNO)
	{
		Log(std::format("OnAgentDBNO={}", bInIsDBNO).c_str());
		return OnAgentDBNOOG(AI, InPlayer, bInIsDBNO);
	}

	void HookAll() {
		MH_CreateHook((LPVOID)(ImageBase + 0x12B1C50), SpawnBot, (LPVOID*)&SpawnBotOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x113F350), GatherNearestLoot, (LPVOID*)&GatherNearestLootOG);

		// I dont think this is right offset
		//MH_CreateHook((LPVOID)(ImageBase + 0xFA9010), SetUpInventoryBot, (LPVOID*)&SetUpInventoryBotOG);

		//HookVTable<UAthenaNavSystem>(44, InitializeForWorld, (LPVOID*)&InitializeForWorldOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x386A430), OnActorBump, (LPVOID*)&OnActorBumpOG);
		//MH_CreateHook((LPVOID)(ImageBase + 0x1D2CED0), OnAgentDBNO, (LPVOID*)&OnAgentDBNOOG); //if we needed it

		Log("Hooked FortAIBotControllerAthena!");
	}
}