// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/ShieldPickup.h"

#include "Character/MultiplayerCharacter.h"
#include "MultiplayerComponent/BuffComponent.h"


AShieldPickup::AShieldPickup()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AShieldPickup::BeginPlay()
{
	Super::BeginPlay();
}

void AShieldPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AShieldPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 29. проверим что это персонаж и запустим таймер восстановление щита 
	MultiplayerCharacter = Cast<AMultiplayerCharacter>(OtherActor);
	if (MultiplayerCharacter)
	{
		MultiplayerCharacter->GetBuffComponent()->ShieldReplenish(ShieldReplenishAmount,ShieldReplenishTime);
	}

	//23.6 при подборе будет запускаться роидтельская функция уничтожающая предмет
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

