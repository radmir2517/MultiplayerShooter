// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"

#include "Character/MultiplayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
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
	// контроллер будет нулевым на всех симуляторах, и будет на игроках которые управляют
	AController* InstigatorController = OwnerCharacter->GetController();
	
	if (!IsValid(OwnerCharacter)) return;
	if (!GetWeaponMesh()->GetSocketByName(SocketNameOnWeapon)) return;
	
	const FTransform SocketTransform = GetWeaponMesh()->GetSocketTransform(SocketNameOnWeapon);
	//10.1 получим начальную точку для HeatScan
	FVector Start = SocketTransform.GetLocation();
	
	FHitResult OutHit;
	// 14.1 Сделаем общую функцию где выполняем трассировку в зависимости нужен ли разброс
	WeaponTraceHit(Start,TargetPoint,OutHit);
	
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
		// 12.1 поправим и сделаем отображение HitEffect и звука на клиенетах
		Client_SpawnHitEffectSound(this,OutHit.Location);
	}
}
void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	const UObject* World = GetWorld();
		
	//14.2 получим конечную точку, если есть разлет то отдельный просчет раслета или несли нет то сделаем ее чуть дальше чтобы трассировку для HitScan сделать
	FVector End = bUseScatters ?  TraceEndWithScatters(TraceStart,HitTarget) : TraceStart + ((HitTarget - TraceStart) * 1.25);
	
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


FVector AHitScanWeapon::TraceEndWithScatters(const FVector& TraceStart, const FVector& HitTarget)
{
	// 13.1 получим вектор единичный от ствола оружие до цели трассировки
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	// 13.2 Найдем центр сферы разлета пуль или дробинок
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	// 13.3 нарисуем сферу разлета
	DrawDebugSphere(GetWorld(),SphereCenter,RadiusSphereScatter,12,FColor::Red,false,5.f);
	// 13.4 Возьмем рандомный в направление вектор и умножим его рандомную величину от 0 до радиуса сферы разлета
	FVector RandomVector = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.f,RadiusSphereScatter);
	// 13.5 прибавим рандом и центр сферы 
	FVector ScatterVector = SphereCenter + RandomVector;
	// 13.7 направление от начало ствола до разлетного вектора
	FVector ToScatterVector = ScatterVector - TraceStart;
	
	// 13.7 нарисуем сферу в месте прибавление рандома
	DrawDebugSphere(GetWorld(),SphereCenter + RandomVector,8.f,12,FColor::Orange,false,5.f);
	//и далее чтобы он был длиннее для трассировки умножим TRACE_LENGHT, но чтобы числа не большие были поделим на длину начального вектора
	DrawDebugLine(GetWorld(),TraceStart,TraceStart + ToScatterVector * TRACE_LENGHT/ToScatterVector.Size(),FColor::Cyan,false, 5.f);
	
	return FVector(TraceStart + ToScatterVector * TRACE_LENGHT / ToScatterVector.Size());
}

