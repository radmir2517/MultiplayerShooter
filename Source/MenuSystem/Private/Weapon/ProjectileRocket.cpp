// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"

#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/RocketMovementComponent.h"


AProjectileRocket::AProjectileRocket()
{
	//7.3 создадим сетку для снаряда
	ProjectileMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	ProjectileMeshComponent->SetupAttachment(CollisionBox);

	// 9.1 установим компонент 
	RocketProjectileMovement = CreateDefaultSubobject<URocketMovementComponent>("RocketProjectileMovement");
	// 9.2 вращение снаряда к его направлению скорости
	RocketProjectileMovement-> bRotationFollowsVelocity = true;
	RocketProjectileMovement->SetIsReplicated(true);
	RocketProjectileMovement-> InitialSpeed = 1500.f;
	RocketProjectileMovement-> MaxSpeed = 1500.f;
	PrimaryActorTick.bCanEverTick = true;
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();
	/*// 8.1.3 запустим эффект дыма
	if (RocketTrailEffect) //  улетел в projectile
	{	//  улетел в projectile
		RocketTrailEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(RocketTrailEffect,GetRootComponent(),FName(),GetActorLocation(),GetActorRotation(),EAttachLocation::KeepWorldPosition,false);
	}*/
	if (RocketMovementSoundLoop)
	{// 8.1.4 запустим звук ракеты летящей
		RocketLoopSoundComponent = UGameplayStatics::SpawnSoundAttached(RocketMovementSoundLoop,GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			RocketLoopAttenuation,
			nullptr,
			true);
	}
}

void AProjectileRocket::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectileRocket::Destroyed()
{	// 9.4 Если уничтожается то уничтожим след, это произойдет после таймера
	if (ProjectileTrailEffectComponent) 
	{
		ProjectileTrailEffectComponent->DestroyComponent();
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner())
	{ // 9.3 Если столкнется с владельцем то не взрываем
		return;
	}
	// 17.1 урон на расстояние сделали функцией и перенсли в базовой класс для других классов
	ApplyExplodeDamage();
	// 8.1 установим таймер на уничтожения снаряда
	if (HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(TimerToDestroy,this,&AProjectileRocket::TimerDestroyedFinished,DestroyTime,false);
	}
	// 8.2 уберем звук летания после стоклновения
	if (RocketLoopSoundComponent)
	{
		RocketLoopSoundComponent->Deactivate();
	}
	if (ProjectileMeshComponent)
	{ // 8.3 после столкнвоения уберем видимость снаряда
		ProjectileMeshComponent->SetVisibility(false);
	}
	if (RocketProjectileMovement)
	{	// 8.4 после столкновения уберем движения снаряда
		RocketProjectileMovement->Deactivate();
	}
	if (HitEffect)
	{ // спавн эффекта вызрыва при попадания снаряда обо что то
		UGameplayStatics::SpawnEmitterAtLocation(this,HitEffect,GetActorLocation(),GetActorRotation());
	}
	if (HitSound)
	{ // если есть звук, то воспроизвести
		UGameplayStatics::PlaySoundAtLocation(this,HitSound,GetActorLocation());
	}
}



