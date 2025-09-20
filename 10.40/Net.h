#pragma once
#include "framework.h"

namespace Net
{
	int GetNetMode()
	{
		//Log("GetNetMode Called!");
		return (int)ENetMode::DedicatedServer;
	}

	void HookAll()
	{
		MH_CreateHook((LPVOID)(ImageBase + 0x2E3E970), GetNetMode, nullptr); //AActor::InternalGetNetMode
		MH_CreateHook((LPVOID)(ImageBase + 0x34D2140), GetNetMode, nullptr); //UWorld::InternalGetNetMode
	}
}