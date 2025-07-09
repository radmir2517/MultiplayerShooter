// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.generated.h"


class UProjectileMovementComponent;
class UBoxComponent;

UCLASS()
class MENUSYSTEM_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();

	virtual void Tick(float DeltaTime) override;
	
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;
	// функция пересечения снаряда
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent,AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UParticleSystem> HitEffect;

	UPROPERTY(EditDefaultsOnly)
	float Damage = 20.f;
};








































