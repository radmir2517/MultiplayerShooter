// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileGrenade.generated.h"

class URocketMovementComponent;

UCLASS()
class MENUSYSTEM_API AProjectileGrenade : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileGrenade();
	virtual void Destroyed() override;
	
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;
	UFUNCTION()
	void OnBounce (const FHitResult& ImpactResult, const FVector& ImpactVelocity);
	//17.1 добавим компонент движения
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<URocketMovementComponent> GrenadeMovementComponent;
private:
	UPROPERTY(EditDefaultsOnly, Category="Projectile")
	TObjectPtr<USoundCue> BounceSound;
	
	// 17.2 внутрення переменная 
	TObjectPtr<USoundAttenuation> BounceAttenuation;
};
