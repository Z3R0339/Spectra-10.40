#pragma once
#include "framework.h"

#include "Bots.h"

namespace NetDriver {
	void (*TickFlushOG)(UNetDriver*, float DeltaSeconds);
	void TickFlush(UNetDriver* Driver, float DeltaSeconds)
	{
		if (!Driver) return;
		if (Driver->ReplicationDriver) ServerReplicateActors(Driver->ReplicationDriver, DeltaSeconds);

		UWorld* World = Driver->World;
		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)World->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)World->GameState;
		if (GameMode->AliveBots.Num() > 0 && GameState->GamePhase > EAthenaGamePhase::Warmup) {
			PlayerBots::TickBots();

			return TickFlushOG(Driver, DeltaSeconds);
		}

		if (Driver->ClientConnections.Num() > 0) {
			if (Globals::bBotsEnabled && GameState->GamePhase > EAthenaGamePhase::Setup) {
				if (GameState->GamePhase == EAthenaGamePhase::Warmup) {
					if ((GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num()) < GameMode->GameSession->MaxPlayers
						&& GameMode->AliveBots.Num() < Globals::MaxBotsToSpawn)
					{
						if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.06f))
						{
							Bots::SpawnPlayerBot();
						}
					}
				}

				PlayerBots::TickBots();
			}
		}

		return TickFlushOG(Driver, DeltaSeconds);
	}

	float GetMaxTickRate()
	{
		return 30.0f;
	}

	void HookAll() {
		MH_CreateHook((LPVOID)(ImageBase + 0x31EECB0), TickFlush, (LPVOID*)&TickFlushOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x3482550), GetMaxTickRate, nullptr);

		Log("NetDriver Hooked!");
	}
}