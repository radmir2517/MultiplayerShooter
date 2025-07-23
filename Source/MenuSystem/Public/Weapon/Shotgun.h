// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HitScanWeapon.h"
#include "Shotgun.generated.h"

UCLASS()
class MENUSYSTEM_API AShotgun : public AHitScanWeapon
{
	GENERATED_BODY()

public:
	AShotgun();

protected:
	virtual void BeginPlay() override;
	virtual void OpenFire(const FVector_NetQuantize& TargetPoint) override;

	
public:
	virtual void Tick(float DeltaTime) override;
};
