// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "SpeedPickup.generated.h"

class AMultiplayerCharacter;

UCLASS()
class MENUSYSTEM_API ASpeedPickup : public APickup
{
	GENERATED_BODY()

public:
	ASpeedPickup();

	virtual void Tick(float DeltaTime) override;

	// функции для поднятия предмета
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
protected:
	virtual void BeginPlay() override;
	
	// 25.1 бафф скорости
	UPROPERTY(EditDefaultsOnly)
	float BaseSpeedBuff = 1600.f;
	// 25.2 бафф скорости когда крадемся
	UPROPERTY(EditDefaultsOnly)
	float CrouchSpeedBuff = 850.f;
	// 25.3 время до сброса баффа
	UPROPERTY(EditDefaultsOnly)
	float SpeedBuffTime = 30.f;

	// 24.0 сделаем указатель на персонажа
	UPROPERTY()
	TObjectPtr<AMultiplayerCharacter> MultiplayerCharacter;
};






























