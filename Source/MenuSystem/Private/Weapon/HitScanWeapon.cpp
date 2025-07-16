// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"

#include "Character/MultiplayerCharacter.h"
#include "Kismet/GameplayStatics.h"


AHitScanWeapon::AHitScanWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AHitScanWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

void AHitScanWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AHitScanWeapon::OpenFire(const FVector_NetQuantize& TargetPoint)
{
	Super::OpenFire(TargetPoint);

	AMultiplayerCharacter* OwnerCharacter = Cast<AMultiplayerCharacter>(GetOwner());
	if (!IsValid(OwnerCharacter)) return;
	
	//10.1 получим начальную точку для HeatScan
	FVector Start = GetWeaponMesh()->GetSocketLocation(SocketNameOnWeapon);
	//10.2 получим конечную точку, сделаем ее чуть дальше чтобы трассировку для HeatScan сделать
	FVector End = Start + ((TargetPoint - Start) * 1.25);

	// 10.3 сделаем трассировку и применим урон
	FHitResult Hit;
	GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility);
	if (Hit.bBlockingHit)
	{
		AMultiplayerCharacter* HitCharacter = Cast<AMultiplayerCharacter>(Hit.GetActor());
		if (HitCharacter)
		{	
			if (HasAuthority())
			{	//10.4 применим урон, делает лишь сервер
				UGameplayStatics::ApplyDamage(HitCharacter,HitScanDamage,OwnerCharacter->Controller,this,UDamageType::StaticClass());
			
				if (HitEffect)
				{ // спавн эффекта вызрыва при попадания снаряда об что то
					UGameplayStatics::SpawnEmitterAtLocation(this,HitEffect,Hit.Location,Hit.ImpactNormal.Rotation());
				}
				if (HitSound)
				{ // если есть звук, то воспроизвести
					UGameplayStatics::PlaySoundAtLocation(this,HitSound,Hit.Location);
				}
			}
		}
	}
}
