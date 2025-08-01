// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileGrenade.h"

#include "NiagaraFunctionLibrary.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Weapon/RocketMovementComponent.h"


AProjectileGrenade::AProjectileGrenade()
{
	PrimaryActorTick.bCanEverTick = true;

	//7.3 создадим сетку для снаряда
	ProjectileMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	ProjectileMeshComponent->SetupAttachment(CollisionBox);
	
	// 17.1 установим компонент 
	GrenadeMovementComponent = CreateDefaultSubobject<URocketMovementComponent>("RocketProjectileMovement");
	// 17.2 вращение снаряда к его направлению скорости
	GrenadeMovementComponent-> bRotationFollowsVelocity = true;
	GrenadeMovementComponent->SetIsReplicated(true);
	GrenadeMovementComponent-> InitialSpeed = 1500.f;
	GrenadeMovementComponent-> MaxSpeed = 1500.f;
	// 17.4 установим чтобы он отскакивал
	GrenadeMovementComponent->bShouldBounce = true;
}

void AProjectileGrenade::Destroyed()
{
	ApplyExplodeDamage();
	Super::Destroyed();
}


void AProjectileGrenade::BeginPlay()
{
	// 17.4 в Projectile ну нас запустить onHit он нам тут не нужен, поэтому запустил акторский beginPlay
	AActor::BeginPlay();

	// 17.5 запустим эффект дыма
	if (ProjectileTrailEffect) 
	{	
		ProjectileTrailEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(ProjectileTrailEffect,GetRootComponent(),FName(),GetActorLocation(),GetActorRotation(),EAttachLocation::KeepWorldPosition,false);
	}
	if (GetWorld())
	{// 17.6 запустим таймер до взрыва гранаты
		GetWorldTimerManager().SetTimer(TimerToDestroy,this,&AProjectileGrenade::TimerDestroyedFinished,DestroyTime,false);
	}
	if (BounceSound)
	{	// 17.7 Проверим если в снаряде затухатель
		BounceAttenuation = BounceSound->AttenuationSettings ? BounceSound->AttenuationSettings : nullptr;
	}
	GrenadeMovementComponent->OnProjectileBounce.AddDynamic(this,&AProjectileGrenade::OnBounce);
}

void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), BounceSound, GetActorLocation(), 1.f,1.f, 0.f,BounceAttenuation);
	}
}

void AProjectileGrenade::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}