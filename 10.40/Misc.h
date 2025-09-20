#pragma once
#include "framework.h"

namespace Misc {
	//inline static void (*KickPlayerOG)(AGameSession*, AController*);
	static void KickPlayer(AGameSession* GameSession, AController* Controller)
	{
		Log(std::format("KickPlayer Called! PlayerName: {}", Controller->PlayerState->GetPlayerName().ToString()).c_str());
		return;
	}

	void (*DispatchRequestOG)(void* a1, void* MCPData, int a3);
	void DispatchRequest(void* a1, void* MCPData, int a3)
	{
		return DispatchRequestOG(a1, MCPData, 3);
	}

	int CanCreateContext(AActor* This)
	{
		//Log("CanCreateContext Called!");
		return 1;
	}

	void HookAll() {
		MH_CreateHook((LPVOID)(ImageBase + 0x30B2B70), KickPlayer, nullptr);
		MH_CreateHook((LPVOID)(ImageBase + 0xBAED60), DispatchRequest, (LPVOID*)&DispatchRequestOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x227D6E0), IsFalse, nullptr); //CollectGarbage
		MH_CreateHook((LPVOID)(ImageBase + 0x15D0330), IsFalse, nullptr); //ChangeGameSessionId

		MH_CreateHook((LPVOID)(ImageBase + 0x22A30C0), CanCreateContext, nullptr); // CanCreateContext (WTF) Tim2025: Found offset 7/17/25

		Log("Hooked Misc!");
	}
}