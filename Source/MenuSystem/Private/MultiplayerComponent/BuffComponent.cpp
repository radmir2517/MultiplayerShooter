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
	HealAmountEverTickTimer = HealAmount / (HealingTime/HealTimerPeriod);
	// 24.4 Проверим что таймер не запущен и запустим его с периодом 0,2сек
	if (!MultiplayerCharacter->GetWorldTimerManager().TimerExists(HealTimer))
	{
		MultiplayerCharacter->GetWorldTimerManager().SetTimer(HealTimer,this ,&UBuffComponent::HandleHealing,HealTimerPeriod,true);
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
		HealingTimeRemaining -= HealTimerPeriod;
	}
}

void UBuffComponent::ShieldReplenish(float InShieldReplenish, float ShieldReplenishTime)
{
	// 29.5 переменные нужные для таймера инициализируем значениями восстановление щита
	ShieldTimeRemaining = ShieldReplenishTime;
	ShieldAmountEverTickTimer = InShieldReplenish / (ShieldReplenishTime/ShieldReplenishTimerPeriod);
	// 29.6 Проверим что таймер не запущен и запустим его с периодом 0,2сек
	if (!MultiplayerCharacter->GetWorldTimerManager().TimerExists(ShieldReplenishTimer))
	{
		MultiplayerCharacter->GetWorldTimerManager().SetTimer(ShieldReplenishTimer,this ,&UBuffComponent::HandleShieldReplenish,ShieldReplenishTimerPeriod,true);
	}
}
void UBuffComponent::HandleShieldReplenish()
{
	// 29.7 если персонаж не жив, или щит полный или восстановление щита закончилось то очистим таймер
	if (!IsValid(MultiplayerCharacter) || MultiplayerCharacter->IsCharacterEliminated() || ShieldTimeRemaining <= 0.f || MultiplayerCharacter->IsCharacterFullShield() )
	{
		MultiplayerCharacter->GetWorldTimerManager().ClearTimer(ShieldReplenishTimer);
	}
	else
	{	// 29.8 если можно восстановить щит то добавим поинтов
		MultiplayerCharacter->AddShieldPoint(ShieldAmountEverTickTimer);
		ShieldTimeRemaining -= ShieldReplenishTimerPeriod;
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

void UBuffComponent::JumpVelocityBuff(float InJumpVelocityBuff, float InJumpBuffTime)
{
	if (MultiplayerCharacter == nullptr || MultiplayerCharacter->GetCharacterMovement() == nullptr) return;

	// 26.11 Установим скорость прыжка
	MultiplayerCharacter->GetCharacterMovement()->JumpZVelocity = InJumpVelocityBuff;
	// 26.12 сделаем это у клиентов также
	MulticastJumpBuff(InJumpVelocityBuff);
	// 26.13 установим таймер на сброс скорости
	MultiplayerCharacter->GetWorldTimerManager().SetTimer(JumpResetBuffTimer,this, &UBuffComponent::ResetJumpBuff,InJumpBuffTime);
}

void UBuffComponent::MulticastJumpBuff_Implementation(float InJumpVelocityBuff)
{
	if (MultiplayerCharacter == nullptr || MultiplayerCharacter->GetCharacterMovement() == nullptr) return;
	// 26.14 Установим скорость прыжка у клиента
	MultiplayerCharacter->GetCharacterMovement()->JumpZVelocity = InJumpVelocityBuff;
}

void UBuffComponent::ResetJumpBuff()
{
	if (MultiplayerCharacter == nullptr || MultiplayerCharacter->GetCharacterMovement() == nullptr) return;
	// 26.15 Установим стандартную скорость
	MultiplayerCharacter->GetCharacterMovement()->JumpZVelocity = InitialJumpVelocity;
	MulticastJumpBuff(InitialJumpVelocity);
}

void UBuffComponent::SetInitialJumpVelocity(float InInitialJumpVelocity)
{
	InitialJumpVelocity = InInitialJumpVelocity;
}




















