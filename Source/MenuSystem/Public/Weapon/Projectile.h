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
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> CollisionBox;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UParticleSystem> HitEffect;

	UPROPERTY(EditDefaultsOnly)
	float Damage = 20.f;

	UPROPERTY(EditDefaultsOnly)  
	TObjectPtr<UNiagaraSystem> RocketTrailEffect;

	//17.1 Сюда запишем указатель на спавнящий эффект чтобы при столкновении его убрать
	UPROPERTY() // улетел projectile
	UNiagaraComponent* RocketTrailEffectComponent; // улетел projectile
	// 17.2 таймер для уничтожения снаряда, а пока он будет спавнить эффект дыма
	UPROPERTY() 
	FTimerHandle TimerToDestroy;

	// 8.3 функция запускания таймеров, уничтожение ракеты
	UFUNCTION()
	void TimerDestroyedFinished();
	// 8.4 время до уничтожения
	UPROPERTY(EditDefaultsOnly)
	float DestroyTime = 3.f;
};








































