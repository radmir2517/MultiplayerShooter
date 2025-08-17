// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/JumpPickup.h"


// Sets default values
AJumpPickup::AJumpPickup()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AJumpPickup::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AJumpPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

