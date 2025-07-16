// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

UCLASS()
class MENUSYSTEM_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	AHitScanWeapon();
	virtual void Tick(float DeltaTime) override;
	// 10.1 переопределим функцию открытия огня
	virtual void OpenFire(const FVector_NetQuantize& TargetPoint) override;
protected:
	virtual void BeginPlay() override;
	// 10.2 уроны обычно в снаряде но из hitScan он будет тут
	UPROPERTY(EditDefaultsOnly, Category="Weapon Properties")
	float HitScanDamage = 20.f;
	// 10.3 добавление эффекта попадания и сзвука попадания
	UPROPERTY(EditDefaultsOnly, Category="Weapon Properties")
	TObjectPtr<UParticleSystem> HitEffect;
	UPROPERTY(EditDefaultsOnly, Category="Weapon Properties")
	TObjectPtr<USoundBase> HitSound;

	
};
