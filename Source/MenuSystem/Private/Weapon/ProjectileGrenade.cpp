// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileGrenade.h"



AProjectileGrenade::AProjectileGrenade()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AProjectileGrenade::BeginPlay()
{
	Super::BeginPlay();
	
}

void AProjectileGrenade::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

