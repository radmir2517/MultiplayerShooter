// Fill out your copyright notice in the Description page of Project Settings.



#include "Weapon/Projectile.h"

#include "Character/MultiplayerCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MenuSystem/MenuSystem.h"

AProjectile::AProjectile()
{
	CollisionBox = CreateDefaultSubobject<UBoxComponent>("CollisionBox");
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Block);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	// установим компонент 
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	// вращение снаряда к его направлению скорости
	ProjectileMovement-> bRotationFollowsVelocity = true;
	ProjectileMovement-> InitialSpeed = 10000.f;
	ProjectileMovement-> MaxSpeed = 10000.f;
	
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (CollisionBox)
	{
		CollisionBox->OnComponentHit.AddDynamic(this,&AProjectile::OnHit);
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                        FVector NormalImpulse, const FHitResult& Hit)
{
	// устарело, т.к мы выполняем его в OnRep_Health и в ReceiveDamage()
	/*AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(OtherActor);
	if (MultiplayerCharacter)
	{
		MultiplayerCharacter->MulticastHitMontagePlay();
	}*/
	Destroy();
}

void AProjectile::Destroyed()
{
	if (HitEffect)
	{ // спавн эффекта вызрыва при попадания снаряда об что то
		UGameplayStatics::SpawnEmitterAtLocation(this,HitEffect,GetActorLocation(),GetActorRotation());
	}
	if (HitSound)
	{ // если есть звук, то воспроизвести
		UGameplayStatics::PlaySoundAtLocation(this,HitSound,GetActorLocation());
	}
	Super::Destroyed();
}













