// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerComponent/BuffComponent.h"

#include "Character/MultiplayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"


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

void UBuffComponent::SpeedBuff(float BaseSpeedBuff, float CrouchSpeedBuff, float SpeedBuffTime)
{
	if (MultiplayerCharacter == nullptr || MultiplayerCharacter->GetCharacterMovement() == nullptr) return;
	// 25.8 назначим повышенную скорость серверу
	MultiplayerCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseSpeedBuff;
	MultiplayerCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeedBuff;
	// 25.9 назначим повышенную скорость клиентам
	MulticastSpeedBuff(BaseSpeedBuff, CrouchSpeedBuff);
	// 25.10 включим таймер который через SpeedBuffTime отключит бафф скорости
	MultiplayerCharacter->GetWorldTimerManager().SetTimer(SpeedResetBuffTimer,this,&UBuffComponent::ResetSpeedBuff,SpeedBuffTime);
}

void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
	if (MultiplayerCharacter == nullptr || MultiplayerCharacter->GetCharacterMovement() == nullptr) return;
	// 25.11 назначим скорость клиентам
	MultiplayerCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	MultiplayerCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

void UBuffComponent::ResetSpeedBuff()
{
	if (MultiplayerCharacter == nullptr || MultiplayerCharacter->GetCharacterMovement() == nullptr) return;
	// 25.12 сброс скорости до начальной
	MultiplayerCharacter->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	MultiplayerCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;
}

void UBuffComponent::SetInitialBaseSpeed(float InInitialBaseSpeed, float InInitialCrouchSpeed)
{
	InitialBaseSpeed = InInitialBaseSpeed;
	InitialCrouchSpeed = InInitialCrouchSpeed;
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
