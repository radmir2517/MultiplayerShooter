// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "JumpPickup.generated.h"

class AMultiplayerCharacter;

UCLASS()
class MENUSYSTEM_API AJumpPickup : public APickup
{
	GENERATED_BODY()

public:
	AJumpPickup();
	
	virtual void Tick(float DeltaTime) override;

	// функции для поднятия предмета
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
protected:
	virtual void BeginPlay() override;

	// 26.1 бафф скорости прыжка
	UPROPERTY(EditDefaultsOnly)
	float JumpVelocityBuff = 3000.f;
	// 26.2 время до сброса баффа
	UPROPERTY(EditDefaultsOnly)
	float JumpBuffTime = 30.f;

	// 26.0 сделаем указатель на персонажа
	UPROPERTY()
	TObjectPtr<AMultiplayerCharacter> MultiplayerCharacter;
};
