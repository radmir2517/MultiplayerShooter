// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Shotgun.h"

#include "Character/MultiplayerCharacter.h"
#include "Kismet/GameplayStatics.h"


AShotgun::AShotgun()
{
	PrimaryActorTick.bCanEverTick = true;
	FireType = EFireType::EFT_Shotgun;
}

void AShotgun::BeginPlay()
{
	Super::BeginPlay();
	
}
/*
void AShotgun::OpenFire(const FVector_NetQuantize& TargetPoint)
{
	//13.1 запустим базовую функцию, не запуская родительскую hitscan
	AWeapon::OpenFire(TargetPoint);

	APawn* OwnerCharacter = Cast<APawn>(GetOwner());
	// контроллер будет нулевым на всех симулторах, ибудет на игроках которые управляют
	AController* InstigatorController = OwnerCharacter->GetController();

	if (!IsValid(OwnerCharacter)) return;
	//13.2 проверим что есть такой сокет
	if (!GetWeaponMesh()->GetSocketByName(SocketNameOnWeapon)) return;
	const FTransform SocketTransform = GetWeaponMesh()->GetSocketTransform(SocketNameOnWeapon);

	FVector Start = SocketTransform.GetLocation();
	// 14,0 Карта в котором будет храниться игроки и кол-во попадание за выстрел
	TMap<AMultiplayerCharacter*, uint32> HitOnCharacters;
	// 13.3 для каждой дробинки запустим рандомную трассировку в пределах радиуса разлета
	for (int32 i = 0; i < CountOfPellets; i++)
	{
		FHitResult OutHit;
		WeaponTraceHit(TargetPoint, OutHit,);

		if (OutHit.bBlockingHit)
		{
			// 11.1 конец дыма будет в месте попадания
			FVector End = OutHit.ImpactPoint;
			AMultiplayerCharacter* HitCharacter = Cast<AMultiplayerCharacter>(OutHit.GetActor());

			if (HitCharacter && InstigatorController && HasAuthority())
			{
				//14.1 применим урон, делает лишь сервер
				if (HitOnCharacters.Contains(HitCharacter))
				{
					// 14.2 если он есть в списке увеличим количество попадании
					HitOnCharacters[HitCharacter]++;
				}
				else
				{
					// 14.3 Если его там нет то добавим его
					HitOnCharacters.Emplace(HitCharacter, 1);
				}
			}
			if (HitEffect)
			{
				// спавн эффекта вызрыва при попадания снаряда об что то
				UGameplayStatics::SpawnEmitterAtLocation(this, HitEffect, OutHit.Location,
				                                         OutHit.ImpactNormal.Rotation());
			}
			if (HitSound)
			{
				// если есть звук, то воспроизвести
				UGameplayStatics::PlaySoundAtLocation(this, HitSound, OutHit.Location);
			}
			if (HasAuthority())
			{
				// 12.1 поправим и сделаем отображение HitEffect и звука на клиенетах
				Client_SpawnHitEffectSound(this, OutHit.Location);
			}
		}
	}
	//14.4 применим урон, считав с карты по кому попали и сколько раз попали
	for (auto Hit : HitOnCharacters)
	{
		if (HasAuthority())
		{
			//14.4 применим урон, считав с карты по кому попали и сколько раз попали
			UGameplayStatics::ApplyDamage(Hit.Key, HitScanDamage * Hit.Value, InstigatorController,
				this, UDamageType::StaticClass());
		}
	}
}
*/

void AShotgun::OpenShotgunFire(const TArray<FVector_NetQuantize>& TargetPoints, const FVector_NetQuantize Start)
{
	//13.1 запустим базовую функцию, не запуская родительскую hitscan
	AWeapon::OpenFire(FVector());

	APawn* OwnerCharacter = Cast<APawn>(GetOwner());
	// контроллер будет нулевым на всех симулторах, ибудет на игроках которые управляют
	AController* InstigatorController = OwnerCharacter->GetController();

	if (!IsValid(OwnerCharacter)) return;
	//13.2 проверим что есть такой сокет
	if (!GetWeaponMesh()->GetSocketByName(SocketNameOnWeapon)) return;
	//const FTransform SocketTransform = GetWeaponMesh()->GetSocketTransform(SocketNameOnWeapon);

	//FVector Start = SocketTransform.GetLocation();
	// 14,0 Карта в котором будет храниться игроки и кол-во попадание за выстрел
	TMap<AMultiplayerCharacter*, uint32> HitOnCharacters;

	for (auto Point : TargetPoints)
	{
		FHitResult OutHit;
		WeaponTraceHit(Point, OutHit, Start);

		if (OutHit.bBlockingHit)
		{
			// 11.1 конец дыма будет в месте попадания
			FVector End = OutHit.ImpactPoint;
			DrawDebugSphere(GetWorld(),End,5.f,5.f,FColor::Red,true);
			AMultiplayerCharacter* HitCharacter = Cast<AMultiplayerCharacter>(OutHit.GetActor());

			if (HitCharacter && InstigatorController && HasAuthority())
			{
				//14.1 применим урон, делает лишь сервер
				if (HitOnCharacters.Contains(HitCharacter))
				{
					// 14.2 если он есть в списке увеличим количество попадании
					HitOnCharacters[HitCharacter]++;
				}
				else
				{
					// 14.3 Если его там нет то добавим его
					HitOnCharacters.Emplace(HitCharacter, 1);
				}
			}
			if (HitEffect)
			{
				// спавн эффекта вызрыва при попадания снаряда об что то
				UGameplayStatics::SpawnEmitterAtLocation(this, HitEffect, OutHit.Location,
														 OutHit.ImpactNormal.Rotation());
			}
			if (HitSound)
			{
				// если есть звук, то воспроизвести
				UGameplayStatics::PlaySoundAtLocation(this, HitSound, OutHit.Location);
			}
			if (HasAuthority())
			{
				// 12.1 поправим и сделаем отображение HitEffect и звука на клиенетах
				Client_SpawnHitEffectSound(this, OutHit.Location);
			}
		}
	}
	
	if (HasAuthority())
	{
		//14.4 применим урон, считав с карты по кому попали и сколько раз попали
		for (auto Hit : HitOnCharacters)
		{
			
			//14.4 применим урон, считав с карты по кому попали и сколько раз попали
			UGameplayStatics::ApplyDamage(Hit.Key, HitScanDamage * Hit.Value, InstigatorController,
					this, UDamageType::StaticClass());
			
		}
	}
}

void AShotgun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

