// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerComponent/BuffComponent.h"

#include "Character/MultiplayerCharacter.h"


UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UBuffComponent::SetMultiplayerCharacter(AMultiplayerCharacter* InMultiplayerCharacter)
{
	MultiplayerCharacter = InMultiplayerCharacter;
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{	// 24.3 переменные нужные для таймера инициализируем значениями лечения
	HealingTimeRemaining = HealingTime;
	HealAmountEverTickTimer = HealAmount / (HealingTime/TimerPeriod);
	// 24.4 Проверим что таймер не запущен и запустим его с периодом 0,2сек
	if (!MultiplayerCharacter->GetWorldTimerManager().TimerExists(HealTimer))
	{
		MultiplayerCharacter->GetWorldTimerManager().SetTimer(HealTimer,this ,&UBuffComponent::HandleHealing,TimerPeriod,true);
	}
}

void UBuffComponent::HandleHealing()
{	// 24.5 если персонаж не жив, или здоровье полное или лечение закончилось то очистим таймер
	if (!IsValid(MultiplayerCharacter) || MultiplayerCharacter->IsCharacterEliminated() || HealingTimeRemaining <= 0.f || MultiplayerCharacter->IsCharacterFullHealthy() )
	{
		MultiplayerCharacter->GetWorldTimerManager().ClearTimer(HealTimer);
	}
	else
	{	// 24.6 если можно лечить то добавим поинтов
		MultiplayerCharacter->AddHealPoint(HealAmountEverTickTimer);
		HealingTimeRemaining -= TimerPeriod;
	}
}