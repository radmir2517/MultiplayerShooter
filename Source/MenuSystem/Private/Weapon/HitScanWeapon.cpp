// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"

#include "Character/MultiplayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"


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

void AHitScanWeapon::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AHitScanWeapon::OpenFire(const FVector_NetQuantize& TargetPoint)
{
	Super::OpenFire(TargetPoint);

	APawn* OwnerCharacter = Cast<APawn>(GetOwner());
	// контроллер будет нулевым на всех симулторах, ибудет на игроках которые управляют
	AController* InstigatorController = OwnerCharacter->GetController();
	
	if (!IsValid(OwnerCharacter)) return;

	const FTransform SocketTransform = GetWeaponMesh()->GetSocketTransform(SocketNameOnWeapon);
	//10.1 получим начальную точку для HeatScan
	FVector Start = SocketTransform.GetLocation();
	//10.2 получим конечную точку, сделаем ее чуть дальше чтобы трассировку для HeatScan сделать
	FVector End = Start + ((TargetPoint - Start) * 1.25);
	// 11.1 Добавим конец дыма если пуля летит в небо
	FVector BeamEnd = End;

	UWorld* World = GetWorld();
	// 10.3 сделаем трассировку и применим урон
	FHitResult Hit;
	GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility);
	if (Hit.bBlockingHit)
	{	// 11.1 конец дыма будет в месте попадания
		End = Hit.ImpactPoint;
		AMultiplayerCharacter* HitCharacter = Cast<AMultiplayerCharacter>(Hit.GetActor());
		if (HitCharacter && InstigatorController)
		{	
			if (HasAuthority())
			{	//10.4 применим урон, делает лишь сервер
				UGameplayStatics::ApplyDamage(HitCharacter,HitScanDamage,InstigatorController,this,UDamageType::StaticClass());
			}
		}
		if (HitEffect)
		{ // спавн эффекта вызрыва при попадания снаряда об что то
			UGameplayStatics::SpawnEmitterAtLocation(this,HitEffect,Hit.Location,Hit.ImpactNormal.Rotation());
		}
		if (HitSound)
		{ // если есть звук, то воспроизвести
			UGameplayStatics::PlaySoundAtLocation(this,HitSound,Hit.Location);
		}
		// 12.1 поправим и сделаем отображение HitEffect и звука на клиенетах
		Client_SpawnHitEffectSound(this,Hit.Location);
	}
	//11.2 проверим что назначена частица
	if (BeamParticles && HasAuthority())
	{	// 11.3 заспавним частицы
		UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(this,BeamParticles,Start,FRotator::ZeroRotator);
		Client_SpawnBeamEffect(World,BeamParticles,Start,BeamEnd);
		if (Beam)
		{	// 11.4 назначим концу дыма координаты
			Beam->SetVectorParameter(FName("Target"),BeamEnd);
		}
	}
}

void AHitScanWeapon::Client_SpawnBeamEffect_Implementation(const UObject* WorldContextObject, UParticleSystem* EmitterTemplate, FVector_NetQuantize Start,FVector_NetQuantize InBeamEnd)
{
	UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(WorldContextObject,BeamParticles,Start,FRotator::ZeroRotator);
		
	if (Beam)
	{	// 11.4 назначим концу дыма координаты
		Beam->SetVectorParameter(FName("Target"),InBeamEnd);
	}
}


void AHitScanWeapon::Client_SpawnHitEffectSound_Implementation(const UObject* WorldContextObject,
	 FVector_NetQuantize HitLocation)
{
	if (HitEffect)
	{ // спавн эффекта вызрыва при попадания снаряда об что то
		UGameplayStatics::SpawnEmitterAtLocation(this,HitEffect,HitLocation);
	}
	if (HitSound)
	{ // если есть звук, то воспроизвести
		UGameplayStatics::PlaySoundAtLocation(this,HitSound,HitLocation);
	}
}