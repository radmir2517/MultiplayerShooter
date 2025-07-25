// Fill out your copyright notice in the Description page of Project Settings.



#include "Weapon/Projectile.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/BoxComponent.h"
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
	
	// 8.1.3 запустим эффект дыма
	if (RocketTrailEffect) //  улетел в projectile
	{	//  улетел в projectile
		RocketTrailEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(RocketTrailEffect,GetRootComponent(),FName(),GetActorLocation(),GetActorRotation(),EAttachLocation::KeepWorldPosition,false);
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                        FVector NormalImpulse, const FHitResult& Hit)
{
	/*// устарело, т.к мы выполняем его в OnRep_Health и в ReceiveDamage()
	AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(OtherActor);
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
	// 17,1 перенесем с rocket. Чтобы воспользоваться в Grenade
	if (RocketTrailEffectComponent) 
	{
		RocketTrailEffectComponent->DestroyComponent();
	}
	Super::Destroyed();
}

void AProjectile::TimerDestroyedFinished()
{
	Destroy();
}












