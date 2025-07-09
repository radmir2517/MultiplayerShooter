// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Casing.h"


// Sets default values
ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetSimulatePhysics(true);
	StaticMesh->SetEnableGravity(true);
	StaticMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();
	// дадим импульс после началы игры
	StaticMesh->AddImpulse(GetActorForwardVector() * CasingImpulseMagnitude * -1.f);
	// назначим время жизни
	SetLifeSpan(LifeSpanValue);
}


