#pragma once
#include "framework.h"

#include "FortPlayerPawn.h"
#include "FortPlayerControllerAthena.h"

#include "SDK/SDK/FortniteGame_parameters.hpp"
using namespace Params;

namespace Hooks
{
	void* (*ProcessEventOG)(UObject*, UFunction*, void*);
	void* ProcessEvent(UObject* Object, UFunction* Function, void* Parameters)
	{
		if (Object && Function)
		{
			if (Function->GetName() == "ServerHandlePickup") FortPlayerPawn::ServerHandlePickup((AFortPlayerPawn*)Object, ((FortPlayerPawn_ServerHandlePickup*)Parameters)->Pickup, ((FortPlayerPawn_ServerHandlePickup*)Parameters)->InFlyTime, ((FortPlayerPawn_ServerHandlePickup*)Parameters)->InStartDirection, ((FortPlayerPawn_ServerHandlePickup*)Parameters)->bPlayPickupSound);
			if (Function->GetName() == "ServerEditBuildingActor") FortPlayerControllerAthena::ServerEditBuildingActor((AFortPlayerControllerAthena*)Object, ((FortPlayerController_ServerEditBuildingActor*)Parameters)->BuildingActorToEdit, ((FortPlayerController_ServerEditBuildingActor*)Parameters)->NewBuildingClass, ((FortPlayerController_ServerEditBuildingActor*)Parameters)->RotationIterations, ((FortPlayerController_ServerEditBuildingActor*)Parameters)->bMirrored);
			if (Function->GetName() == "ServerCreateBuildingActor") FortPlayerControllerAthena::ServerCreateBuildingActor((AFortPlayerControllerAthena*)Object, ((FortPlayerController_ServerCreateBuildingActor*)Parameters)->CreateBuildingData);
			if (Function->GetName() == "ServerBeginEditingBuildingActor") FortPlayerControllerAthena::ServerBeginEditingBuildingActor((AFortPlayerController*)Object, ((FortPlayerController_ServerBeginEditingBuildingActor*)Parameters)->BuildingActorToEdit);
			if (Function->GetName() == "ServerEndEditingBuildingActor") FortPlayerControllerAthena::ServerEndEditingBuildingActor((AFortPlayerController*)Object, ((FortPlayerController_ServerEndEditingBuildingActor*)Parameters)->BuildingActorToStopEditing);
			if (Function->GetName() == "OnCapsuleBeginOverlap") FortPlayerPawn::OnCapsuleBeginOverlap((AFortPlayerPawn*)Object, ((FortPlayerPawn_OnCapsuleBeginOverlap*)Parameters)->OverlappedComp, ((FortPlayerPawn_OnCapsuleBeginOverlap*)Parameters)->OtherActor, ((FortPlayerPawn_OnCapsuleBeginOverlap*)Parameters)->OtherComp, ((FortPlayerPawn_OnCapsuleBeginOverlap*)Parameters)->OtherBodyIndex, ((FortPlayerPawn_OnCapsuleBeginOverlap*)Parameters)->bFromSweep, ((FortPlayerPawn_OnCapsuleBeginOverlap*)Parameters)->SweepResult);
			if (Function->GetName() == "ServerMove") FortPlayerPawn::ServerMove((AFortPhysicsPawn*)Object, ((FortPhysicsPawn_ServerMove*)Parameters)->InState);
		}

		return ProcessEventOG(Object, Function, Parameters);
	}

	void HookAll()
	{
		MH_CreateHook(reinterpret_cast<void*>(ImageBase+Offsets::ProcessEvent), ProcessEvent, reinterpret_cast<void**>(&ProcessEventOG));
	}
}