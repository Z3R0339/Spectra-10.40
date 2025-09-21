#pragma once
#include "framework.h"

#include "Bots.h"

namespace NetDriver {
	void (*TickFlushOG)(UNetDriver*, float DeltaSeconds);
	void TickFlush(UNetDriver* Driver, float DeltaSeconds)
	{
		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		//if (!Driver->ReplicationDriver) Log("ReplicationDriver Don't Eixst!");
		if (Driver->ReplicationDriver) ServerReplicateActors(Driver->ReplicationDriver, DeltaSeconds);

		if (GameState->GamePhase == EAthenaGamePhase::Warmup &&
			GameMode->AlivePlayers.Num() > 0
			&& (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num()) < GameMode->GameSession->MaxPlayers
			&& GameMode->AliveBots.Num() < Globals::MaxBotsToSpawn && Globals::bBotsEnabled)
		{
			if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.06f))
			{
				Bots::SpawnPlayerBot();
			}
		}

		if (Globals::bBotsEnabled && GameState->GamePhase > EAthenaGamePhase::Setup) {
			PlayerBots::TickBots();
		}

		return TickFlushOG(Driver, DeltaSeconds);
	}

	float GetMaxTickRate()
	{
		return 60.0f;
	}

	void HookAll() {
		MH_CreateHook((LPVOID)(ImageBase + 0x31EECB0), TickFlush, (LPVOID*)&TickFlushOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x3482550), GetMaxTickRate, nullptr);

		Log("NetDriver Hooked!");
	}
}