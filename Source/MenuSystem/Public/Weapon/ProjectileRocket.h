// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

class URocketMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class MENUSYSTEM_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileRocket();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;
	
protected:
	virtual void BeginPlay() override;
	
	// 7.1 функция пересечения снаряда
	virtual void OnHit(UPrimitiveComponent* HitComponent,AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	// 8.1 Сделаем указатели на эффект следа ракеты и для звука
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UNiagaraSystem> RocketTrailEffect;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundBase> RocketMovementSoundLoop;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundAttenuation> RocketLoopAttenuation;
	// 8.2 таймер для уничтожения снаряда, а пока он будет спавнить эффект дыма
	UPROPERTY()
	FTimerHandle TimerToDestroy;
	// 9.1 установим тут новый наш компонент
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<URocketMovementComponent> RocketProjectileMovement;
private:
	// 8.3 функция запускания таймеров, уничтожение ракеты
	UFUNCTION()
	void TimerDestroyedFinished();
	// 8.4 время до уничтожения
	UPROPERTY(EditDefaultsOnly)
	float DestroyTime = 3.f;

	
	//7.2 создадим сетку для снаряда
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	// 8.5 Сюда запишем указатель на спавнящий эффект чтобы при столкновении его убрать
	UPROPERTY()
	UNiagaraComponent* RocketTrailEffectComponent;
	// 8.6 Сюда запишем указатель на спавнящий звук чтобы при столкновении его убрать
	UPROPERTY()
	UAudioComponent* RocketLoopSoundComponent;
};


































































