#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <intrin.h>
#include <sstream>
#include <array>
#include <tlhelp32.h>
#include <future>
#include <set>
#include <variant>
#include <unordered_set>

#include "minhook/MinHook.h"
#include "SDK/SDK.hpp"
#include "Globals.h"

#pragma comment(lib, "minhook/minhook.lib")

using namespace SDK;

static auto ImageBase = InSDKUtils::GetImageBase();

static UFortBuildingItemDefinition* Floor = UObject::FindObject<UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Floor.BuildingItemData_Floor");
static UFortBuildingItemDefinition* Roof = UObject::FindObject<UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_RoofS.BuildingItemData_RoofS");
static UFortBuildingItemDefinition* Stair = UObject::FindObject<UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Stair_W.BuildingItemData_Stair_W");
static UFortBuildingItemDefinition* Wall = UObject::FindObject<UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Wall.BuildingItemData_Wall");
static UFortEditToolItemDefinition* EditTool = UObject::FindObject<UFortEditToolItemDefinition>("FortEditToolItemDefinition EditTool.EditTool");

static std::vector<UAthenaCharacterItemDefinition*> Characters{};
static std::vector<UAthenaPickaxeItemDefinition*> Pickaxes{};
static std::vector<UAthenaBackpackItemDefinition*> Backpacks{};
static std::vector<UAthenaGliderItemDefinition*> Gliders{};
static std::vector<UAthenaSkyDiveContrailItemDefinition*> Contrails{};
inline std::vector<UAthenaDanceItemDefinition*> Dances{};

static int32 (*ServerReplicateActors)(UReplicationDriver*, float DeltaSeconds) = decltype(ServerReplicateActors)(ImageBase + 0xA33E90); //UReplicationGraph

static UNetDriver* (*CreateNetDriver)(UEngine* This, UWorld* InWorld, FName NetDriverDefinition) = decltype(CreateNetDriver)(ImageBase + 0x347FAF0);
static bool (*InitListen)(UNetDriver* This, UWorld* InWorld, FURL& ListenURL, bool bReuseAddressAndPort, FString& Error) = decltype(InitListen)(ImageBase + 0x6F5F90);
static void (*SetWorld)(UNetDriver* This, UWorld* InWorld) = decltype(SetWorld)(ImageBase + 0x31EDF40);
static bool (*InitHost)(UObject* Beacon) = decltype(InitHost)(ImageBase + 0x6F5A30);
static void (*PauseBeaconRequests)(UObject* Beacon, bool bPause) = decltype(PauseBeaconRequests)(ImageBase + 0x17F03D0);

static void(*GiveAbilityOG)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec) = decltype(GiveAbilityOG)(ImageBase + 0x935010);
static void (*AbilitySpecConstructor)(FGameplayAbilitySpec*, UGameplayAbility*, int, int, UObject*) = decltype(AbilitySpecConstructor)(ImageBase + 0x958F90);
static bool (*InternalTryActivateAbility)(UAbilitySystemComponent* AbilitySystemComp, FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey InPredictionKey, UGameplayAbility** OutInstancedAbility, void* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData) = decltype(InternalTryActivateAbility)(ImageBase + 0x9367F0);
static FGameplayAbilitySpecHandle(*GiveAbilityAndActivateOnce)(UAbilitySystemComponent* ASC, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec) = decltype(GiveAbilityAndActivateOnce)(ImageBase + 0x935130);

static FVector* (*PickSupplyDropLocationOG)(AFortAthenaMapInfo* MapInfo, FVector* outLocation, __int64 Center, float Radius) = decltype(PickSupplyDropLocationOG)(ImageBase + 0x1177CE0);

inline static ABuildingSMActor* (*ReplaceBuildingActor)(ABuildingSMActor* BuildingSMActor, unsigned int a2, UObject* a3, unsigned int a4, int a5, bool bMirrored, AFortPlayerControllerAthena* PC) = decltype(ReplaceBuildingActor)(ImageBase + 0x13D0DE0);
static __int64 (*CantBuild)(UWorld*, UObject*, FVector, FRotator, char, void*, char*) = decltype(CantBuild)(ImageBase + 0x1601820);

static void* (*ApplyCharacterCustomization)(void* a1, void* a2) = decltype(ApplyCharacterCustomization)(ImageBase + 0x1A1F5F0);

static void (*BotManagerSetup)(__int64 BotManaager, __int64 Pawn, __int64 BehaviorTree, __int64 a4, DWORD* SkillLevel, __int64 a7, __int64 StartupInventory, __int64 BotNameSettings, __int64 a10, BYTE* CanRespawnOnDeath, unsigned __int8 BitFieldDataThing, BYTE* CustomSquadId) = decltype(BotManagerSetup)(ImageBase + 0x12AA150);

static void(*RemoveFromAlivePlayers)(AFortGameModeAthena*, AFortPlayerControllerAthena*, APlayerState*, AFortPlayerPawn*, UFortWeaponItemDefinition*, uint8_t DeathCause, char) = decltype(RemoveFromAlivePlayers)(ImageBase + 0x11D95E0);
static void (*AddToAlivePlayers)(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* Player) = decltype(AddToAlivePlayers)(ImageBase + 0x11B47F0);

static void* (*StaticFindObjectOG)(UClass*, UObject* Package, const wchar_t* OrigInName, bool ExactClass) = decltype(StaticFindObjectOG)(ImageBase + 0x22FB1E0);
static void* (*StaticLoadObjectOG)(UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename, uint32_t LoadFlags, UObject* Sandbox, bool bAllowObjectReconciliation, void*) = decltype(StaticLoadObjectOG)(ImageBase + 0x22FC4C0);

static void (*OnRep_ZiplineState)(AFortPlayerPawn* a1) = decltype(OnRep_ZiplineState)(ImageBase + 0x1967D30);

AFortAthenaMutator_Bots* BotMutator = nullptr;
TArray<FVector> PickedSupplyDropLocations;
TArray<APlayerController*> GivenLootPlayers;

static TArray<AActor*> BuildingFoundations;
static TArray<AActor*> PlayerStarts;

// text manipulation utils
namespace TextManipUtils {
	// Found this from stack overflow :fire:
	std::vector<std::string> SplitWhitespace(std::string const& input) {
		std::istringstream buffer(input);
		std::vector<std::string> ret;

		std::copy(std::istream_iterator<std::string>(buffer),
			std::istream_iterator<std::string>(),
			std::back_inserter(ret));
		return ret;
	}
}

void Log(const std::string& msg)
{
	static bool firstCall = true;

	if (firstCall)
	{
		std::ofstream logFile("Spectra_log.txt", std::ios::trunc);
		if (logFile.is_open())
		{
			logFile << "[SPECTRA]: Log file initialized!\n";
			logFile.close();
		}
		firstCall = false;
	}

	std::ofstream logFile("Spectra_log.txt", std::ios::app);
	if (logFile.is_open())
	{
		logFile << "[SPECTRA]: " << msg << std::endl;
		logFile.close();
	}

	std::cout << "[SPECTRA]: " << msg << std::endl;
}

template<typename T>
void HookVTable(int Idx, void* Detour, void** OG = nullptr)
{
	void** VTable = reinterpret_cast<void**>(T::GetDefaultObj()->VTable);
	if (OG)
		*OG = VTable[Idx];

	DWORD OldProtect;
	if (VirtualProtect(&VTable[Idx], 8, PAGE_EXECUTE_READWRITE, &OldProtect))
	{
		VTable[Idx] = Detour;
		VirtualProtect(&VTable[Idx], 8, OldProtect, &OldProtect);
	}
}

inline FQuat RotatorToQuat(FRotator Rotation)
{
	FQuat Quat;
	const float DEG_TO_RAD = 3.14159f / 180.0f;
	const float DIVIDE_BY_2 = DEG_TO_RAD / 2.0f;

	float SP = sin(Rotation.Pitch * DIVIDE_BY_2);
	float CP = cos(Rotation.Pitch * DIVIDE_BY_2);
	float SY = sin(Rotation.Yaw * DIVIDE_BY_2);
	float CY = cos(Rotation.Yaw * DIVIDE_BY_2);
	float SR = sin(Rotation.Roll * DIVIDE_BY_2);
	float CR = cos(Rotation.Roll * DIVIDE_BY_2);

	Quat.X = CR * SP * SY - SR * CP * CY;
	Quat.Y = -CR * SP * CY - SR * CP * SY;
	Quat.Z = CR * CP * SY - SR * SP * CY;
	Quat.W = CR * CP * CY + SR * SP * SY;

	return Quat;
}

template <typename T>
static inline T* StaticFindObject(std::wstring ObjectName)
{
	return (T*)StaticFindObjectOG(T::StaticClass(), nullptr, ObjectName.c_str(), false);
}

template<typename T>
inline T* Cast(UObject* Object)
{
	if (!Object || !Object->IsA(T::StaticClass()))
		return nullptr;
	return (T*)Object;
}

template<typename T = UObject>
static inline T* StaticLoadObject(const std::string& Name)
{
	auto ConvName = std::wstring(Name.begin(), Name.end());

	T* Object = StaticFindObject<T>(ConvName);

	if (!Object)
	{
		Object = (T*)StaticLoadObjectOG(T::StaticClass(), nullptr, ConvName.c_str(), nullptr, 0, nullptr, false, nullptr);
	}

	return Object;
}

template<typename T>
T* GetDefaultObject()
{
	return (T*)T::StaticClass()->DefaultObject;
}

static inline FQuat FRotToQuat(FRotator Rotation) {
	FQuat Quat;
	const float DEG_TO_RAD = 3.14159f / 180.0f;
	const float DIVIDE_BY_2 = DEG_TO_RAD / 2.0f;

	float SP = sin(Rotation.Pitch * DIVIDE_BY_2);
	float CP = cos(Rotation.Pitch * DIVIDE_BY_2);
	float SY = sin(Rotation.Yaw * DIVIDE_BY_2);
	float CY = cos(Rotation.Yaw * DIVIDE_BY_2);
	float SR = sin(Rotation.Roll * DIVIDE_BY_2);
	float CR = cos(Rotation.Roll * DIVIDE_BY_2);

	Quat.X = CR * SP * SY - SR * CP * CY;
	Quat.Y = -CR * SP * CY - SR * CP * SY;
	Quat.Z = CR * CP * SY - SR * SP * CY;
	Quat.W = CR * CP * CY + SR * SP * SY;

	return Quat;
}

template<typename T>
inline T* SpawnActor(FVector Loc, FRotator Rot = FRotator(), AActor* Owner = nullptr, SDK::UClass* Class = T::StaticClass(), ESpawnActorCollisionHandlingMethod Handle = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
{
	FTransform Transform{};
	Transform.Scale3D = FVector{ 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = FRotToQuat(Rot);

	return (T*)UGameplayStatics::FinishSpawningActor(UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, Handle, Owner), Transform);
}

template<typename T>
static inline T* SpawnAActor(FVector Loc = { 0,0,0 }, FRotator Rot = { 0,0,0 }, AActor* Owner = nullptr)
{
	FTransform Transform{};
	Transform.Scale3D = { 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = FRotToQuat(Rot);

	AActor* NewActor = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), T::StaticClass(), Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner);
	return (T*)UGameplayStatics::FinishSpawningActor(NewActor, Transform);
}

template<typename T>
static inline T* SpawnActorClass(FVector Loc = { 0,0,0 }, FRotator Rot = { 0,0,0 }, UClass* Class = nullptr, AActor* Owner = nullptr)
{
	FTransform Transform{};
	Transform.Scale3D = { 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = RotatorToQuat(Rot);

	AActor* NewActor = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner);
	return (T*)UGameplayStatics::FinishSpawningActor(NewActor, Transform);
}

template<typename T>
T* Actors(UClass* Class = T::StaticClass(), FVector Loc = {}, FRotator Rot = {}, AActor* Owner = nullptr)
{
	return SpawnActor<T>(Loc, Rot, Owner, Class);
}

AActor* DuplicateActor(AActor* Original)
{
	if (!Original) return nullptr;

	const FVector Location = Original->K2_GetActorLocation();
	const FRotator Rotation = Original->K2_GetActorRotation();
	const FTransform Transform = Original->GetTransform();

	auto* Class = Original->Class;
	auto* Owner = Original->GetOwner();

	auto* Duplicated = (AActor*)UGameplayStatics::FinishSpawningActor(
		UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, Owner),
		Transform
	);

	if (Duplicated)
	{
		//FName DupedTag = UKismetStringLibrary::Conv_StringToName(L"_ServerSpawned_");

		Duplicated->Name = Original->Name;
		Duplicated->Tags = Original->Tags;

		//Duplicated->Tags.Add(DupedTag);
	}

	FGameplayTag* OriginalFactionTag = (FGameplayTag*)((uintptr_t)Original + 0xB0);
	FGameplayTag* DuplicateFactionTag = (FGameplayTag*)((uintptr_t)Duplicated + 0xB0);

	if (OriginalFactionTag && DuplicateFactionTag)
	{
		//Log(OriginalFactionTag->TagName.ToString());
		*DuplicateFactionTag = *OriginalFactionTag;
	}

	return Duplicated;
}

AFortPickup* SpawnPickup(UFortItemDefinition* ItemDef, int OverrideCount, int LoadedAmmo, FVector Loc, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source, AFortPawn* Pawn = nullptr)
{
	auto SpawnedPickup = Actors<AFortPickup>(AFortPickup::StaticClass(), Loc);
	SpawnedPickup->bRandomRotation = true;

	auto& PickupEntry = SpawnedPickup->PrimaryPickupItemEntry;
	PickupEntry.ItemDefinition = ItemDef;
	PickupEntry.Count = OverrideCount;
	PickupEntry.LoadedAmmo = LoadedAmmo;
	PickupEntry.ReplicationKey++;
	SpawnedPickup->OnRep_PrimaryPickupItemEntry();
	SpawnedPickup->PawnWhoDroppedPickup = Pawn;

	SpawnedPickup->TossPickup(Loc, Pawn, -1, true, SourceType, Source);

	SpawnedPickup->SetReplicateMovement(true);
	SpawnedPickup->MovementComponent = (UProjectileMovementComponent*)GetDefaultObject<UGameplayStatics>()->SpawnObject(UProjectileMovementComponent::StaticClass(), SpawnedPickup);

	if (SourceType == EFortPickupSourceTypeFlag::Container)
	{
		SpawnedPickup->bTossedFromContainer = true;
		SpawnedPickup->OnRep_TossedFromContainer();
	}

	return SpawnedPickup;
}

std::map<AFortPickup*, float> PickupLifetimes;
AFortPickup* SpawnStack(APlayerPawn_Athena_C* Pawn, UFortItemDefinition* Def, int Count, bool giveammo = false, int ammo = 0)
{
	auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

	FVector Loc = Pawn->K2_GetActorLocation();
	AFortPickup* Pickup = Actors<AFortPickup>(AFortPickup::StaticClass(), Loc);
	Pickup->bReplicates = true;
	PickupLifetimes[Pickup] = Statics->GetTimeSeconds(UWorld::GetWorld());
	Pickup->PawnWhoDroppedPickup = Pawn;
	Pickup->PrimaryPickupItemEntry.Count = Count;
	Pickup->PrimaryPickupItemEntry.ItemDefinition = Def;
	if (giveammo)
	{
		Pickup->PrimaryPickupItemEntry.LoadedAmmo = ammo;
	}
	Pickup->PrimaryPickupItemEntry.ReplicationKey++;

	Pickup->OnRep_PrimaryPickupItemEntry();
	Pickup->TossPickup(Loc, Pawn, 6, true, EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::Unset);

	Pickup->MovementComponent = (UProjectileMovementComponent*)Statics->SpawnObject(UProjectileMovementComponent::StaticClass(), Pickup);
	Pickup->MovementComponent->bReplicates = true;
	((UProjectileMovementComponent*)Pickup->MovementComponent)->SetComponentTickEnabled(true);

	return Pickup;
}

static AFortPickup* SpawnPickup(FFortItemEntry ItemEntry, FVector Location, EFortPickupSourceTypeFlag PickupSource = EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource SpawnSource = EFortPickupSpawnSource::Unset)
{
	auto Pickup = SpawnPickup(ItemEntry.ItemDefinition, ItemEntry.Count, ItemEntry.LoadedAmmo, Location, PickupSource, SpawnSource);
	return Pickup;
}

inline void ShowFoundation(ABuildingFoundation* BuildingFoundation) {
	if (!BuildingFoundation)
		return;

	BuildingFoundation->bServerStreamedInLevel = true;
	BuildingFoundation->DynamicFoundationType = EDynamicFoundationType::Static;
	BuildingFoundation->OnRep_ServerStreamedInLevel();

	BuildingFoundation->FoundationEnabledState = EDynamicFoundationEnabledState::Enabled;
	BuildingFoundation->DynamicFoundationRepData.EnabledState = EDynamicFoundationEnabledState::Enabled;
	BuildingFoundation->DynamicFoundationTransform = BuildingFoundation->GetTransform();
	BuildingFoundation->OnRep_DynamicFoundationRepData();
}

FVector PickSupplyDropLocation(SDK::AFortAthenaMapInfo* MapInfo, SDK::FVector Center, float Radius)
{
	if (!PickSupplyDropLocationOG)
		return SDK::FVector(0, 0, 0);

	const float MinDistance = 10000.0f;

	for (int i = 0; i < 20; i++)
	{
		SDK::FVector loc = FVector(0, 0, 0);
		PickSupplyDropLocationOG(MapInfo, &loc, (__int64)&Center, Radius);

		bool bTooClose = false;
		for (const auto& other : PickedSupplyDropLocations)
		{
			float dx = loc.X - other.X;
			float dy = loc.Y - other.Y;
			float dz = loc.Z - other.Z;

			float distSquared = dx * dx + dy * dy + dz * dz;

			if (distSquared < MinDistance * MinDistance)
			{
				bTooClose = true;
				break;
			}
		}

		if (!bTooClose)
		{
			PickedSupplyDropLocations.Add(loc);
			return loc;
		}
	}

	return SDK::FVector(0, 0, 0);
}

template<typename T>
inline std::vector<T*> GetAllObjectsOfClass(UClass* Class = T::StaticClass())
{
	std::vector<T*> Objects{};

	for (int i = 0; i < UObject::GObjects->Num(); ++i)
	{
		UObject* Object = UObject::GObjects->GetByIndex(i);

		if (!Object)
			continue;

		if (Object->GetFullName().contains("Default"))
			continue;

		if (Object->GetFullName().contains("Test"))
			continue;

		if (Object->IsA(Class) && !Object->IsDefaultObject())
		{
			Objects.push_back((T*)Object);
		}
	}

	return Objects;
}

int CountActorsWithName(FName TargetName, UClass* Class)
{
	TArray<AActor*> FoundActors;
	auto* Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;
	Statics->GetAllActorsOfClass(UWorld::GetWorld(), Class, &FoundActors);

	int Count = 0;
	for (AActor* Actor : FoundActors)
	{
		if (Actor && Actor->GetName() == TargetName.ToString())
			Count++;
	}
	return Count;
}

AFortPlayerControllerAthena* GetPCFromId(FUniqueNetIdRepl& ID)
{
	for (auto& PlayerState : UWorld::GetWorld()->GameState->PlayerArray)
	{
		auto PlayerStateAthena = Cast<AFortPlayerStateAthena>(PlayerState);
		if (!PlayerStateAthena)
			continue;
		if (PlayerStateAthena->AreUniqueIDsIdentical(ID, PlayerState->UniqueId))
			return Cast<AFortPlayerControllerAthena>(PlayerState->Owner);
	}

	return nullptr;
}

static inline void* _NpFH = nullptr;
template <typename _Ot = void*>
__forceinline static void ExecHook(UFunction* _Fn, void* _Detour, _Ot& _Orig = _NpFH)
{
	if (!_Fn)
		return;
	if (!std::is_same_v<_Ot, void*>)
		_Orig = (_Ot)_Fn->ExecFunction;

	_Fn->ExecFunction = reinterpret_cast<UFunction::FNativeFuncPtr>(_Detour);
}

template <typename _Ot = void*>
__forceinline static void ExecHook(const char* _Name, void* _Detour, _Ot& _Orig = _NpFH)
{
	UFunction* _Fn = StaticLoadObject<UFunction>(_Name);
	ExecHook(_Fn, _Detour, _Orig);
}

enum EFortGamePlaylistAthena
{
	SOLO,
	DUOS,
	SQUADS
};

/** Types of net modes that we know about - synced with EngineBaseTypes.h */
enum class ENetMode
{
	Standalone,
	DedicatedServer,
	ListenServer,
	Client,

	MAX,
};

int IsTrue()
{
	return TRUE;
}

int IsFalse()
{
	return FALSE;
}

void nullFunc() 
{

}

FRotator Rotator(FQuat This) {
	const double SingularityTest = This.Z * This.X - This.W * This.Y;
	const double YawY = 2. * (This.W * This.Z + This.X * This.Y);
	const double YawX = (1. - 2. * ((This.Y * This.Y) + (This.Z * This.Z)));

	const double SINGULARITY_THRESHOLD = 0.4999995;
	const double RAD_TO_DEG = 57.29577951308232;
	FRotator RotatorFromQuat{};

	if (SingularityTest < -SINGULARITY_THRESHOLD)
	{
		RotatorFromQuat.Pitch = -90.;
		RotatorFromQuat.Yaw = atan2(YawY, YawX) * RAD_TO_DEG;
		RotatorFromQuat.Roll = FRotator::NormalizeAxis(-RotatorFromQuat.Yaw - (2. * atan2(This.X, This.W) * RAD_TO_DEG));
	}
	else if (SingularityTest > SINGULARITY_THRESHOLD)
	{
		RotatorFromQuat.Pitch = 90.;
		RotatorFromQuat.Yaw = atan2(YawY, YawX) * RAD_TO_DEG;
		RotatorFromQuat.Roll = FRotator::NormalizeAxis(RotatorFromQuat.Yaw - (2. * atan2(This.X, This.W) * RAD_TO_DEG));
	}
	else
	{
		RotatorFromQuat.Pitch = asin(2. * SingularityTest) * RAD_TO_DEG;
		RotatorFromQuat.Yaw = atan2(YawY, YawX) * RAD_TO_DEG;
		RotatorFromQuat.Roll = atan2(-2. * (This.W * This.X + This.Y * This.Z), (1. - 2. * ((This.X * This.X) + (This.Y * This.Y)))) * RAD_TO_DEG;
	}

	return RotatorFromQuat;
}