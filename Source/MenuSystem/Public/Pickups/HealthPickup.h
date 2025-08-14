// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "HealthPickup.generated.h"

class AMultiplayerCharacter;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class MENUSYSTEM_API AHealthPickup : public APickup
{
	GENERATED_BODY()

public:
	AHealthPickup();
	
	virtual void Tick(float DeltaTime) override;
	// 23.5 Переопредлим функции
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;
	// 23.3 сделаем переменную которая будет основной визуальной частью аля вроде меша будет
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNiagaraComponent> HealthPickupComponent;
	// 23.4 эффект активируемый после поднятия предмета
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UNiagaraSystem> HealingEffect;
	
	// 23.1 Создадим переменную для значения лечения
	UPROPERTY(EditDefaultsOnly)
	float HealAmount = 100.f;
	// 23.2 Создадим переменную времени лечения чтобы это было не сразу
	UPROPERTY(EditDefaultsOnly)
	float HealingTime = 5.f;
	
	// 24.0 сделаем указатель на персонажа
	UPROPERTY()
	TObjectPtr<AMultiplayerCharacter> MultiplayerCharacter;
};











































