#pragma once
#include "framework.h"

namespace PE
{
	void* (*ProcessEventOG)(UObject*, UFunction*, void*);
	void* ProcessEvent(UObject* Object, UFunction* Function, void* Parameters)
	{
		return ProcessEventOG(Object, Function, Parameters);
	}

	void HookAll()
	{
		MH_CreateHook(reinterpret_cast<void*>(ImageBase+Offsets::ProcessEvent), ProcessEvent, reinterpret_cast<void**>(&ProcessEventOG));

		Log("PE Hooked!");
	}
}