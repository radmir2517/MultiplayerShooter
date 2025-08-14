// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/Pickup_Ammo.h"

#include "Character/MultiplayerCharacter.h"
#include "MultiplayerComponent/CombatComponent.h"

APickup_Ammo::APickup_Ammo()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APickup_Ammo::BeginPlay()
{
	Super::BeginPlay();
}

void APickup_Ammo::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickup_Ammo::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(OtherActor);
	if (MultiplayerCharacter)
	{
		if (MultiplayerCharacter->GetCombatComponent())
		{
			MultiplayerCharacter->GetCombatComponent()->PickUpAmmo(WeaponType, AmmoAmount);
		}
	}
	
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

