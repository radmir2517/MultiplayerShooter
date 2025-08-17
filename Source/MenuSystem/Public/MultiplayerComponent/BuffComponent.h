// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


class AMultiplayerCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MENUSYSTEM_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuffComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
	//22.1 сеттер для персонажа
	void SetMultiplayerCharacter(AMultiplayerCharacter* InMultiplayerCharacter);
	// 24.2.6 Функция лечения 
	void Heal(float HealAmount, float HealingTime);
	// 25.4 здесь будем скороять персонажа сервера и клиента и запускать таймер сброса
	void SpeedBuff(float BaseSpeedBuff, float CrouchSpeedBuff, float SpeedBuffTime);
	// 25.5  получим стандартную скорость чтобы потом к нему вернуть
	void SetInitialBaseSpeed(float InInitialBaseSpeed, float InInitialCrouchSpeed);

	// 26.4 Функция усиления прыжка
	void JumpVelocityBuff(float InJumpVelocityBuff, float InJumpBuffTime);
	// 26.5 получение стандартных значения скорости прыжка
	void SetInitialJumpVelocity(float InInitialJumpVelocity);
protected:
	virtual void BeginPlay() override;
	// 25.6 Функция которая и у клиентов назначит изменения скорости
	UFUNCTION(NetMulticast,Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);
	// 25.7 Функция сброса скорости к нормали запуская таймеров
	void ResetSpeedBuff();

	// 26.7 Функция которая и у клиентов назначит изменения скорости прыжка
	UFUNCTION(NetMulticast,Reliable)
	void MulticastJumpBuff(float InJumpVelocityBuff);
	// 26.8 Функция сброса скорости прыжка к нормали запуская таймеров
	void ResetJumpBuff();
	
	// 22.0 Указатель для персонажа
	UPROPERTY()
	TObjectPtr<AMultiplayerCharacter> MultiplayerCharacter;

private:
	
	/*
	 * Лечение
	 */
	// 24.2.1 Сделаем таймер для лечения каждые 0.2 сек
	FTimerHandle HealTimer;
	// 24.2.2 переменная = HealingTime  которая будет вычитать таймер пока не станет 0
	float HealingTimeRemaining;
	// 24.2.3 период таймера
	UPROPERTY(EditDefaultsOnly)
	float HealTimerPeriod = 0.2;
	// 24.2.4 кол-во восстановение за раз таймером
	float HealAmountEverTickTimer;
	// 24.2.5 Функция которая будет запускать таймер лечения
	void HandleHealing();
	/*
	 * Ускорение
	 */
	//25.5.1 Базовая скорость ходьбы, назначается сюда в начале игры
	float InitialBaseSpeed;
	//25.5.2 Базовая скорость на корточках, назначается сюда в начале игры
	float InitialCrouchSpeed;
	//25.5.3 Таймер который сбросит скорость до нормального 
	FTimerHandle SpeedResetBuffTimer;

	/*
	* Усиления прыжка
	*/
	// 26.4.1 период таймера
	UPROPERTY(EditDefaultsOnly)
	float JumpTimerPeriod = 0.2;
	//26.9 стандартная скорость прыжка, назначается сюда в начале игры
	float InitialJumpVelocity;
	//26.10 Таймер который сбросит скорость до нормального 
	FTimerHandle JumpResetBuffTimer;
};

















