// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/RocketMovementComponent.h"


// Sets default values for this component's properties
URocketMovementComponent::URocketMovementComponent()
{

	PrimaryComponentTick.bCanEverTick = true;

}



void URocketMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void URocketMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

UProjectileMovementComponent::EHandleBlockingHitResult URocketMovementComponent::HandleBlockingHit(
	const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);
	// 8.1 после столкновения вернем статус что можно продолжать движение
	return EHandleBlockingHitResult::AdvanceNextSubstep;
}

void URocketMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	Super::HandleImpact(Hit, TimeSlice, MoveDelta);
}
