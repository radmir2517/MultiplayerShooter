// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/JumpPickup.h"

#include "Character/MultiplayerCharacter.h"
#include "MultiplayerComponent/BuffComponent.h"


AJumpPickup::AJumpPickup()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AJumpPickup::BeginPlay()
{
	Super::BeginPlay();
	
}

void AJumpPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AJumpPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 26.3 проверим что это персонаж и запустим ускорение прыжка и таймер сброса
	MultiplayerCharacter = Cast<AMultiplayerCharacter>(OtherActor);
	// 26.4 Запустим бафф прыжка у сервера и у клиентов и запустим таймер сброса скорости
	if (MultiplayerCharacter)
	{
		MultiplayerCharacter->GetBuffComponent()->JumpVelocityBuff(JumpVelocityBuff,JumpBuffTime);
	}
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

