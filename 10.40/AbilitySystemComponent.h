#pragma once
#include "framework.h"

namespace AbilitySystemComponent
{
	//https://github.com/EpicGames/UnrealEngine/blob/87f8792983fb4228be213b15b57f675dfe143d16/Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp#L584
	FGameplayAbilitySpec* FindAbilitySpecFromHandle(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle)
	{
		for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->ActivatableAbilities.Items)
		{
			if (Spec.Handle == Handle)
				return &Spec;
		}

		return nullptr;
	}

	//https://github.com/EpicGames/UnrealEngine/blob/87f8792983fb4228be213b15b57f675dfe143d16/Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp#L1445
	void InternalServerTryActiveAbility(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, const FPredictionKey& PredictionKey, const FGameplayEventData* TriggerEventData)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilitySystemComponent, Handle);
		if (!Spec)
		{
			AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
			return;
		}

		const UGameplayAbility* AbilityToActivate = Spec->Ability;

		UGameplayAbility* InstancedAbility = nullptr;
		Spec->InputPressed = true;

		if (!InternalTryActivateAbility(AbilitySystemComponent, Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
		{
			AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
			Spec->InputPressed = false;

			//AbilitySystemComponent->ActivatableAbilities.MarkArrayDirty();
			AbilitySystemComponent->ActivatableAbilities.MarkItemDirty(*Spec);
		}
	}

	void GiveAbility(UFortGameplayAbility* Ability, AFortPlayerState* PS)
	{
		FGameplayAbilitySpec Spec{};
		AbilitySpecConstructor(&Spec, Ability, 1, -1, nullptr);
		GiveAbilityOG(PS->AbilitySystemComponent, &Spec.Handle, Spec);
	}

	void GiveAbilitySet(UFortAbilitySet* Ability, AFortPlayerState* PS)
	{
		for (int i = 0; i < Ability->GameplayAbilities.Num(); i++)
			GiveAbility((UFortGameplayAbility*)Ability->GameplayAbilities[i].Get()->DefaultObject, PS);
		for (int i = 0; i < Ability->GrantedGameplayEffects.Num(); i++) {
			FGameplayEffectApplicationInfoHard GameplayEffect = Ability->GrantedGameplayEffects[i];
			PS->AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(GameplayEffect.GameplayEffect.Get(), GameplayEffect.Level, PS->AbilitySystemComponent->MakeEffectContext());
		}
	}

	void HookAll()
	{
		int InternalServerTryActiveAbilityIndex = 0xF6;

		HookVTable<UAbilitySystemComponent>(InternalServerTryActiveAbilityIndex, InternalServerTryActiveAbility);
		HookVTable<UFortAbilitySystemComponent>(InternalServerTryActiveAbilityIndex, InternalServerTryActiveAbility);
		HookVTable<UFortAbilitySystemComponentAthena>(InternalServerTryActiveAbilityIndex, InternalServerTryActiveAbility);
	}
}