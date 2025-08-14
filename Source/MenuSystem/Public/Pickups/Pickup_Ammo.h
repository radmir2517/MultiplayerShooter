// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "GameplayTypes/WeaponTypes.h"
#include "Pickup_Ammo.generated.h"


UCLASS()
class MENUSYSTEM_API APickup_Ammo : public APickup
{
	GENERATED_BODY()

public:
	APickup_Ammo();

	virtual void Tick(float DeltaTime) override;

	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly)
	EWeaponType WeaponType;
	UPROPERTY(EditDefaultsOnly)
	int32 AmmoAmount = 30;
	
};
