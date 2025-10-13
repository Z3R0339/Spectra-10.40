#pragma once
#include "framework.h"
#include "ServerBotManager.h"

#include "FortInventory.h"
#include "AbilitySystemComponent.h"

namespace GameMode
{
	bool (*ReadyToStartMatchOG)(AGameMode*);
	bool ReadyToStartMatch(AFortGameModeAthena* GameMode)
	{
		static bool bSetupPlaylist = false;
		static bool bInitialize = false;
		static bool bIsNetReady = false;
		UFortPlaylistAthena* Playlist = nullptr;

		AFortGameStateAthena* GameState = reinterpret_cast<AFortGameStateAthena*>(GameMode->GameState);
		if (!GameMode || !GameState) return ReadyToStartMatchOG(GameMode);

		if (!bSetupPlaylist)
		{
			bSetupPlaylist = true;
			Log(std::format("bSetupPlaylist = {}", bSetupPlaylist).c_str());

			if (Globals::bEventEnabled) {
				Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Music/Playlist_Music_High.Playlist_Music_High");
			}
			else {
				Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Playlist_DefaultDuo.Playlist_DefaultDuo");  //StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo");
			}


			int32 PlaylistId = Playlist->PlaylistId;

			Log(std::format("Playlist = {}", Playlist->GetName()).c_str());

			GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;

			GameMode->CurrentPlaylistId = PlaylistId;
			GameMode->CurrentPlaylistName = Playlist->PlaylistName;

			GameState->bGameModeWillSkipAircraft = Playlist->bSkipAircraft;
			GameState->CachedSafeZoneStartUp = Playlist->SafeZoneStartUp;
			GameState->AirCraftBehavior = Playlist->AirCraftBehavior;

			GameMode->GameSession->SessionName = UKismetStringLibrary::Conv_StringToName(L"GameSession");
			GameMode->GameSession->MaxPlayers = Playlist->MaxPlayers;

			GameState->CurrentPlaylistId = PlaylistId;

			Globals::MaxPlayersPerTeam = Playlist->MaxSquadSize;
			Globals::NextTeamIndex = Playlist->DefaultFirstTeam;
			Globals::NextBotTeamIndex = Playlist->DefaultLastTeam; //We be the same team as the bots

			//Log(std::format("Globals::MaxPlayersPerTeam={}", Globals::MaxPlayersPerTeam).c_str());
		}

		if (!GameState->MapInfo) return ReadyToStartMatchOG(GameMode);

		if (!bInitialize)
		{
			bInitialize = true;
			Log(std::format("bInitialize = {}", bInitialize).c_str());

			GameState->OnRep_CurrentPlaylistId();
			GameState->OnRep_CurrentPlaylistInfo();

			GameMode->bWorldIsReady = true;
			GameMode->bAllowSpectateAfterDeath = true;
			GameMode->MinRespawnDelay = 5.0f;
			GameMode->WarmupRequiredPlayerCount = 1;
			((UNavigationSystemV1*)UWorld::GetWorld()->NavigationSystem)->bAutoCreateNavigationData = true;

			if (auto BotManager = (UFortServerBotManagerAthena*)UGameplayStatics::SpawnObject(UFortServerBotManagerAthena::StaticClass(), GameMode))
			{
				GameMode->ServerBotManager = BotManager;
				BotManager->CachedGameState = GameState;
				BotManager->CachedGameMode = GameMode;

				BotMutator = SpawnActor<AFortAthenaMutator_Bots>({});
				BotManager->CachedBotMutator = BotMutator;
				BotMutator->CachedGameMode = GameMode;
				BotMutator->CachedGameState = GameState;

				if (!GameMode->AIDirector) {
					Log("No AIDirector, Creating one automatically...");
					AAthenaAIDirector* Director = SpawnActor<AAthenaAIDirector>({});
					GameMode->AIDirector = Director;
					GameMode->AIDirector->Activate();
				}

				AFortAIGoalManager* GoalManager = SpawnActor<AFortAIGoalManager>({});
				GameMode->AIGoalManager = GoalManager;

				//this is better (we wont have stuff that goes null anymore + dev stuff check)
				for (size_t i = 0; i < UObject::GObjects->Num(); i++)
				{
					UObject* Obj = UObject::GObjects->GetByIndex(i);
					if (Obj && Obj->IsA(UAthenaCharacterItemDefinition::StaticClass()))
					{
						std::string SkinsData = ((UAthenaCharacterItemDefinition*)Obj)->Name.ToString();

						if (SkinsData.contains("Athena_Commando") || SkinsData.contains("CID_Character") || !SkinsData.contains("CID_NPC") || !SkinsData.contains("CID_VIP") || !SkinsData.contains("CID_TBD"))
						{
							Characters.push_back((UAthenaCharacterItemDefinition*)Obj);
						}
					}
					if (Obj && Obj->IsA(UAthenaBackpackItemDefinition::StaticClass())) // pretty sure this doesnt have dev stuff
					{
						Backpacks.push_back((UAthenaBackpackItemDefinition*)Obj);
					}
					if (Obj && Obj->IsA(UAthenaPickaxeItemDefinition::StaticClass())) // pretty sure this doesnt have dev stuff
					{
						Pickaxes.push_back((UAthenaPickaxeItemDefinition*)Obj);
					}
					if (Obj && Obj->IsA(UAthenaDanceItemDefinition::StaticClass()))
					{
						std::string EmoteData = ((UAthenaDanceItemDefinition*)Obj)->Name.ToString();

						if (EmoteData.contains("EID") || !EmoteData.contains("Sync") || !EmoteData.contains("Owned"))
						{

							Dances.push_back((UAthenaDanceItemDefinition*)Obj);
						}

					}
					if (Obj && Obj->IsA(UAthenaGliderItemDefinition::StaticClass()))
					{
						Gliders.push_back((UAthenaGliderItemDefinition*)Obj);
					}
				}

				Log("Initialised Bots!");
			}
			else
			{
				Log("BotManager is nullptr!");
			}
		}

		if (!bIsNetReady)
		{
			bIsNetReady = true;
			Log(std::format("bIsNetReady = {}", bIsNetReady).c_str());

			FName NetDriverDefinition = UKismetStringLibrary::Conv_StringToName(L"GameNetDriver");
			UNetDriver* Driver = CreateNetDriver(UEngine::GetEngine(), UWorld::GetWorld(), NetDriverDefinition);

			Driver->NetDriverName = NetDriverDefinition;
			Driver->World = UWorld::GetWorld();

			FString Error = FString();
			FURL NewURL{};
			NewURL.Port = 7777;

			if (!InitListen(Driver, UWorld::GetWorld(), NewURL, true, Error))
				Log(std::format("UIpNetDriver::InitListen Error: {}", Error.ToString()).c_str());

			UWorld::GetWorld()->NetDriver = Driver;
			SetWorld(Driver, UWorld::GetWorld());
			
			for (int i = 0; i < UWorld::GetWorld()->LevelCollections.Num(); i++)
				UWorld::GetWorld()->LevelCollections[i].NetDriver = Driver;

			Log(std::format("IpNetDriver listening on port {}!", NewURL.Port).c_str());
			SetConsoleTitleW(std::format(L"Spectra 10.40 | IpNetDriver listening on port {}!", NewURL.Port).c_str());
		}

		if (ReadyToStartMatchOG(GameMode)) {
			UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &PlayerStarts);
			UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ABuildingFoundation::StaticClass(), &BuildingFoundations);
			for (AActor* BuildingFoundation : BuildingFoundations) {
				FVector Loc = BuildingFoundation->K2_GetActorLocation();
			}

			return true;
		}
	}

	APawn* SpawnDefaultPawnFor(AGameModeBase* GameMode, AController* NewPlayer, AActor* StartSpot)
	{
		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)NewPlayer;
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		AFortPlayerPawnAthena* Pawn = (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(NewPlayer, StartSpot->GetTransform());

		ApplyCharacterCustomization(PlayerState, Pawn);

		return Pawn;
	}

	void (*HandleStartingNewPlayerOG)(AGameModeBase* This, AFortPlayerControllerAthena* NewPlayer);
	void HandleStartingNewPlayer(AGameModeBase* This, AFortPlayerControllerAthena* NewPlayer)
	{
		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		//APlayerPawn_Athena_C* NewPawn = SpawnAActor<APlayerPawn_Athena_C>(NewPlayer->K2_GetActorLocation(), NewPlayer->K2_GetActorRotation());
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)NewPlayer->PlayerState;

		/*NewPlayer->Possess(NewPawn);

		NewPawn->Controller = NewPlayer;
		NewPawn->OnRep_Controller();*/

		NewPlayer->bHasClientFinishedLoading = true;
		NewPlayer->bHasServerFinishedLoading = true;
		NewPlayer->OnRep_bHasServerFinishedLoading();

		PlayerState->bHasFinishedLoading = true;
		PlayerState->bHasStartedPlaying = true;
		PlayerState->OnRep_bHasStartedPlaying();

		NewPlayer->ClientQuickBars = SpawnAActor<AFortQuickBars>();
		NewPlayer->ClientQuickBars->SetOwner(NewPlayer);

		PlayerState->SquadId = PlayerState->TeamIndex - 3;
		PlayerState->OnRep_SquadId();

		FGameMemberInfo Member;
		Member.MostRecentArrayReplicationKey = -1;
		Member.ReplicationID = -1;
		Member.ReplicationKey = -1;
		Member.TeamIndex = PlayerState->TeamIndex;
		Member.SquadId = PlayerState->SquadId;
		Member.MemberUniqueId = PlayerState->UniqueId;

		GameState->GameMemberInfoArray.Members.Add(Member);
		GameState->GameMemberInfoArray.MarkItemDirty(Member);

		if (!NewPlayer->MatchReport)
		{
			NewPlayer->MatchReport = (UAthenaPlayerMatchReport*)UGameplayStatics::SpawnObject(UAthenaPlayerMatchReport::StaticClass(), NewPlayer);
		}

		for (int i = 0; i < GameMode->StartingItems.Num(); i++)
			FortInventory::GiveItem(NewPlayer, GameMode->StartingItems[i].Item, GameMode->StartingItems[i].Count);

		UAthenaPickaxeItemDefinition* PickaxeDefinition = nullptr;
		FFortAthenaLoadout& CosmeticLoadoutPC = NewPlayer->CosmeticLoadoutPC;

		PickaxeDefinition = CosmeticLoadoutPC.Pickaxe != nullptr ? CosmeticLoadoutPC.Pickaxe : StaticLoadObject<UAthenaPickaxeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");

		if (PickaxeDefinition)
			FortInventory::GiveItem(NewPlayer, PickaxeDefinition->WeaponDefinition, 1);

		AbilitySystemComponent::GiveAbilitySet(StaticLoadObject<UFortAbilitySet>("/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer"), PlayerState);

		FortInventory::Update(NewPlayer);

		return HandleStartingNewPlayerOG(GameMode, NewPlayer);
	}

	static inline void (*OriginalOnAircraftExitedDropZone)(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft);
	void OnAircraftExitedDropZone(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft)
	{
		Log("OnAircraftExitedDropZone!");

		if (Globals::bBotsEnabled) {
			for (auto PlayerBot : PlayerBots::PlayerBotArray) {
				if (!PlayerBot->bHasJumpedFromBus) {
					AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

					PlayerBot->Pawn->K2_TeleportTo(GameState->GetAircraft(0)->K2_GetActorLocation(), {});
					PlayerBot->Pawn->BeginSkydiving(true);
					PlayerBot->bHasJumpedFromBus = true;

					PlayerBot->BotState = PlayerBots::EBotState::Skydiving;
				}
			}
		}

		// When we do the actual event we should set it to the timer
		if (Globals::bEventEnabled)
		{
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

		return OriginalOnAircraftExitedDropZone(GameMode, FortAthenaAircraft);
	}

	__int64 (*OnAircraftEnteredDropZoneOG)(AFortGameModeAthena*);
	__int64 OnAircraftEnteredDropZone(AFortGameModeAthena* a1)
	{
		Log("OnAircraftEnteredDropZone Called!");
		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		GameState->GamePhaseStep = EAthenaGamePhaseStep::BusFlying;

		return OnAircraftEnteredDropZoneOG(a1);
	}

	EFortTeam (*PickTeamOG)(AFortGameModeAthena* GameMode, uint8_t PreferredTeam, AFortPlayerControllerAthena* Controller);
	EFortTeam PickTeam(AFortGameModeAthena* GameMode, uint8_t PreferredTeam, AFortPlayerControllerAthena* Controller)
	{
		int Ret = Globals::NextTeamIndex;

		++Globals::CurrentPlayersOnTeam;

		if (Globals::CurrentPlayersOnTeam == Globals::MaxPlayersPerTeam)
		{
			Globals::NextTeamIndex++;
			Globals::CurrentPlayersOnTeam = 0;
		}

		Log("PickTeam Called!");
		return EFortTeam(Ret);
	}

	void HookAll()
	{
		MH_CreateHook((LPVOID)(ImageBase + 0x11D8640), ReadyToStartMatch, (LPVOID*)&ReadyToStartMatchOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x11E1180), SpawnDefaultPawnFor, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x15BEDE0), HandleStartingNewPlayer, (LPVOID*)&HandleStartingNewPlayerOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x11CF710), OnAircraftExitedDropZone, (LPVOID*)&OriginalOnAircraftExitedDropZone);

		MH_CreateHook((LPVOID)(ImageBase + 0x11CF670), OnAircraftEnteredDropZone, (LPVOID*)&OnAircraftEnteredDropZoneOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x11D42B0), PickTeam, (LPVOID*)&PickTeamOG);

		Log("Hooked GameMode!");
	}
}