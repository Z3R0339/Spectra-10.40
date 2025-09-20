#pragma once
#include "framework.h"

namespace Vehicles {
	void SpawnVehicles()
	{
		UGameplayStatics* Statics = UGameplayStatics::GetDefaultObj();

		TArray<AActor*> Spawners;

		UClass* Class = AFortAthenaVehicleSpawner::StaticClass();

		Statics->GetAllActorsOfClass(UWorld::GetWorld(), Class, &Spawners);

		for (int i = 0; i < Spawners.Num(); i++)
		{
			AFortAthenaVehicleSpawner* Vehicle = (AFortAthenaVehicleSpawner*)Spawners[i];
			SpawnActorClass<AFortAthenaVehicleSpawner>(Vehicle->K2_GetActorLocation(), Vehicle->K2_GetActorRotation(), Vehicle->GetVehicleClass());
		}

		Spawners.Free();
	}
}