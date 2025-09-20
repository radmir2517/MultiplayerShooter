// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"

#include "Character/MultiplayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"


AHitScanWeapon::AHitScanWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	FireType = EFireType::EFT_HitScan;
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
	// контроллер будет нулевым на всех симуляторах, и будет на игроках которые управляют
	AController* InstigatorController = OwnerCharacter->GetController();
	
	if (!IsValid(OwnerCharacter)) return;

	const FTransform SocketTransform = GetWeaponMesh()->GetSocketTransform(SocketNameOnWeapon);
	//10.1 получим начальную точку для HeatScan
	FVector TraceStart = SocketTransform.GetLocation();
	
	FHitResult OutHit;
	// 14.1 Сделаем общую функцию где выполняем трассировку в зависимости нужен ли разброс
	WeaponTraceHit(TargetPoint,OutHit,TraceStart);
	
	if (OutHit.bBlockingHit)
	{
		AMultiplayerCharacter* HitCharacter = Cast<AMultiplayerCharacter>(OutHit.GetActor());
		
		if (HitCharacter && InstigatorController && HasAuthority())
		{	//10.4 применим урон, делает лишь сервер
			UGameplayStatics::ApplyDamage(HitCharacter,HitScanDamage,InstigatorController,this,UDamageType::StaticClass());
		}
		if (HitEffect)
		{ // спавн эффекта вызрыва при попадания снаряда об что то
			UGameplayStatics::SpawnEmitterAtLocation(this,HitEffect,OutHit.Location,OutHit.ImpactNormal.Rotation());
		}
		if (HitSound)
		{ // если есть звук, то воспроизвести
			UGameplayStatics::PlaySoundAtLocation(this,HitSound,OutHit.Location);
		}
		if (HasAuthority())
		{
			// 12.1 поправим и сделаем отображение HitEffect и звука на клиенетах
			Client_SpawnHitEffectSound(this,OutHit.Location);
		}
		
	}
}
void AHitScanWeapon::WeaponTraceHit(const FVector& HitTarget, FHitResult& OutHit, const FVector_NetQuantize& TraceStart)
{
	const UObject* World = GetWorld();

	if (!GetWeaponMesh()->GetSocketByName(SocketNameOnWeapon)) return;
	
	//const FTransform SocketTransform = GetWeaponMesh()->GetSocketTransform(SocketNameOnWeapon);
	//10.1 получим начальную точку для HeatScan
	//FVector TraceStart = SocketTransform.GetLocation();
	//14.2 получим конечную точку,  сделаем ее чуть дальше чтобы трассировку для HitScan сделать
	FVector End = TraceStart + ((HitTarget - TraceStart) * 1.25);
	
	GetWorld()->LineTraceSingleByChannel(OutHit,TraceStart,End,ECC_Visibility);
	if (OutHit.bBlockingHit)
	{
		End = OutHit.ImpactPoint;
	}
	// 11.1 Добавим конец дыма если пуля летит в небо
	FVector BeamEnd = End;
	
	//11.2 проверим что назначена частица дыма
	if (BeamParticles && HasAuthority())
	{	// 11.3 заспавним частицы дыма
		UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(this,BeamParticles,TraceStart,FRotator::ZeroRotator);
		Client_SpawnBeamEffect(World,BeamParticles,TraceStart,BeamEnd);
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




