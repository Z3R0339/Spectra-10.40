#pragma once
#include "framework.h"
#include "FortAIBotControllerAthena.h"

namespace Net
{
	//inline static void (*KickPlayerOG)(AGameSession*, AController*);
	static void KickPlayer(AGameSession* GameSession, AController* Controller)
	{
		Log(std::format("KickPlayer Called! PlayerName: {}", Controller->PlayerState->GetPlayerName().ToString()).c_str());
		return;
	}

	int GetNetMode()
	{
		//Log("GetNetMode Called!");
		return (int)ENetMode::DedicatedServer;
	}

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
			if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.05f))
			{
				FortAIBotControllerAthena::SpawnPlayerBot();
			}
		}

		if (Globals::bBotsEnabled && GameState->GamePhase > EAthenaGamePhase::Setup) {
			PlayerBots::TickBots();
		}

		return TickFlushOG(Driver, DeltaSeconds);
	}

	float GetMaxTickRate()
	{
		return 30.0f;
	}

	// I think a1 should be the aactor that is getting spawned
	int CanCreateContext(__int64 a1) 
	{
		//Log("CanCreateContext Called!");
		return 1;
	}

	void HookAll()
	{
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x30B2B70), KickPlayer, nullptr);
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x2E3E970), GetNetMode, nullptr); //AActor::InternalGetNetMode
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x34D2140), GetNetMode, nullptr); //UWorld::InternalGetNetMode
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x31EECB0), TickFlush, reinterpret_cast<void**>(&TickFlushOG));
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x3482550), GetMaxTickRate, nullptr);
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x227D6E0), IsFalse, nullptr); //CollectGarbage
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x15D0330), IsFalse, nullptr); //ChangeGameSessionId

		// Kicks player whyy??
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x22A30C0), CanCreateContext, nullptr); // CanCreateContext (WTF) Tim2025: Found offset 7/17/25
	}
}