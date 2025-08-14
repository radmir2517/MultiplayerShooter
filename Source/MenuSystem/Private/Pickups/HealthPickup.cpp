// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/HealthPickup.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/MultiplayerCharacter.h"
#include "MultiplayerComponent/BuffComponent.h"


AHealthPickup::AHealthPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	//23.6 Создадим компонент который будет как мешка у нас анимированная
	HealthPickupComponent = CreateDefaultSubobject<UNiagaraComponent>("HealthPickupComponent");
	HealthPickupComponent->SetupAttachment(GetRootComponent());
}

void AHealthPickup::BeginPlay()
{
	Super::BeginPlay();
	
}

void AHealthPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AHealthPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{	// 24.1 проверим что это персонаж и запустим таймер лечения 
	MultiplayerCharacter = Cast<AMultiplayerCharacter>(OtherActor);
	if (MultiplayerCharacter)
	{
		MultiplayerCharacter->GetBuffComponent()->Heal(HealAmount,HealingTime);
	}

	//23.6 при подборе будет запускаться роидтельская функция уничтожающая предмет
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void AHealthPickup::Destroyed()
{	//23.7 перед уничтожения воспроизведем эффект и в родительской еще и звук будет
	if (HealingEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this,HealingEffect,GetActorLocation(),GetActorRotation());
	}
	
	Super::Destroyed();
}
