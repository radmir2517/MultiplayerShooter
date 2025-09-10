// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "ProjectileWeapon.generated.h"

class AProjectile;
/**
 * 
 */
UCLASS()
class MENUSYSTEM_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()
	
public:
	AProjectileWeapon();
	// функция расчета местоположения спавна пули , не используется, перенесен в OpenFire
	//virtual FTransform CalculateSpawnBulletTransform(const FVector& TargetPoint);
	// функция спавна пули
	virtual void OpenFire(const FVector_NetQuantize& TargetPoint) override;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AProjectile> ProjectileClass;
	
};
