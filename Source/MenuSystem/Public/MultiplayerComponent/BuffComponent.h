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
protected:
	virtual void BeginPlay() override;
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
	float TimerPeriod = 0.2;
	// 24.2.4 кол-во восстановение за раз таймером
	float HealAmountEverTickTimer;
	// 24.2.5 Функция которая будет запускать таймер лечения
	void HandleHealing();
};

















