// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "RocketMovementComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MENUSYSTEM_API URocketMovementComponent : public UProjectileMovementComponent
{
	GENERATED_BODY()

public:
	URocketMovementComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
protected:
	virtual void BeginPlay() override;
	// 9.1 переопредилим функцию определение столкновение и остановки движения
	virtual EHandleBlockingHitResult HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining) override;
	// 9.2 просто переопределим, оставим суперскуую функцию 
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0, const FVector& MoveDelta = FVector::ZeroVector) override;
};








































