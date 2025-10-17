// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.generated.h"


class UNiagaraComponent;
class UNiagaraSystem;
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
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBoxComponent> CollisionBox;

	//17.3 создадим сетку для снаряда. перенесли с класса Rocket
	UPROPERTY(VisibleAnywhere)	
	TObjectPtr<UStaticMeshComponent> ProjectileMeshComponent; 
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UParticleSystem> HitEffect;

	UPROPERTY(EditDefaultsOnly)
	float Damage = 20.f;

	UPROPERTY(EditDefaultsOnly)  
	TObjectPtr<UNiagaraSystem> ProjectileTrailEffect;

	//17.1 Сюда запишем указатель на спавнящий эффект чтобы при столкновении его убрать . перенесли с класса Rocket
	UPROPERTY()  
	UNiagaraComponent* ProjectileTrailEffectComponent; 
	// 17.2 таймер для уничтожения снаряда, а пока он будет спавнить эффект дыма . перенесли с класса Rocket
	UPROPERTY()
	// 17.2 таймер для уничтожения снаряда, а пока он будет спавнить эффект дыма . перенесли с класса Rocket
	FTimerHandle TimerToDestroy;

	UPROPERTY(EditDefaultsOnly)  
	float DamageInnerRadius = 200.f;
	UPROPERTY(EditDefaultsOnly)  
	float DamageOuterRadius = 500.f;
	
	void ApplyExplodeDamage();

	// 8.3 функция запускания таймеров, уничтожение ракеты . перенесли с класса Rocket
	UFUNCTION()
	void TimerDestroyedFinished();
	// 8.4 время до уничтожения . перенесли с класса Rocket
	UPROPERTY(EditDefaultsOnly)
	float DestroyTime = 3.f;

	/*
	 * Server side Rewind
	 */
	//34,1 назначим переменные 
	UPROPERTY(EditDefaultsOnly)
	float InitialSpeed = 15000.f;
	UPROPERTY(EditDefaultsOnly)
	bool bUseServerSideRewind = false;
};








































