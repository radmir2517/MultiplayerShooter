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

	void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	// 10.1 переопределим функцию открытия огня
	virtual void OpenFire(const FVector_NetQuantize& TargetPoint) override;
protected:
	virtual void BeginPlay() override;
	// 10.2 уроны обычно в снаряде но из hitScan он будет тут
	UPROPERTY(EditDefaultsOnly, Category="Weapon Properties")
	float HitScanDamage = 20.f;
	// 10.3 добавление эффекта попадания и звука попадания
	UPROPERTY(EditDefaultsOnly, Category="Weapon Properties")
	TObjectPtr<UParticleSystem> HitEffect;
	UPROPERTY(EditDefaultsOnly, Category="Weapon Properties")
	TObjectPtr<USoundBase> HitSound;
	// 11.1 Трассер для пистолета
	UPROPERTY(EditDefaultsOnly, Category="Weapon Properties")
	TObjectPtr<UParticleSystem> BeamParticles;

	
private:
	// 11.2 Функция спавна эффекта для клиента
	UFUNCTION(Client, Unreliable)
	void Client_SpawnBeamEffect(const UObject* WorldContextObject, UParticleSystem* EmitterTemplate, FVector_NetQuantize Start,FVector_NetQuantize BeamEnd);
	UFUNCTION(CLient, Unreliable)
	void Client_SpawnHitEffectSound(const UObject* WorldContextObject, FVector_NetQuantize HitLocation);
};













































