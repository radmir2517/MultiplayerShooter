// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/SpeedPickup.h"

#include "Character/MultiplayerCharacter.h"
#include "MultiplayerComponent/BuffComponent.h"


ASpeedPickup::ASpeedPickup()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASpeedPickup::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASpeedPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASpeedPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 25. проверим что это персонаж и запустим ускорение и таймер сброса
	MultiplayerCharacter = Cast<AMultiplayerCharacter>(OtherActor);
	// 25. Запустим бафф скорости у сервера и у клиентов и запустим таймер сброса скорости
	if (MultiplayerCharacter)
	{
		MultiplayerCharacter->GetBuffComponent()->SpeedBuff(BaseSpeedBuff, CrouchSpeedBuff, SpeedBuffTime);
	}
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

