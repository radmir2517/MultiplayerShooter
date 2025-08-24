// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "ShieldPickup.generated.h"

class AMultiplayerCharacter;

UCLASS()
class MENUSYSTEM_API AShieldPickup : public APickup
{
	GENERATED_BODY()

public:
	AShieldPickup();
	virtual void Tick(float DeltaTime) override;
	// 29.3 Переопредлим функции
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;


protected:
	virtual void BeginPlay() override;
		
	// 29.1 Создадим переменную для значения щита
	UPROPERTY(EditDefaultsOnly)
	float ShieldReplenishAmount = 100.f;
	// 29.2 Создадим переменную времени восстановление щита 
	UPROPERTY(EditDefaultsOnly)
	float ShieldReplenishTime = 5.f;
	
	// 29.0 сделаем указатель на персонажа
	UPROPERTY()
	TObjectPtr<AMultiplayerCharacter> MultiplayerCharacter;
	
};
