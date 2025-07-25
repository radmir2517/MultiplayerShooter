// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileGrenade.generated.h"

UCLASS()
class MENUSYSTEM_API AProjectileGrenade : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileGrenade();

	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;

	
};
