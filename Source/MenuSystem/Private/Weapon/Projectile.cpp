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
	if (ProjectileTrailEffect) 
	{	
		ProjectileTrailEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(ProjectileTrailEffect,GetRootComponent(),FName(),GetActorLocation(),GetActorRotation(),EAttachLocation::KeepWorldPosition,false);
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
	if (ProjectileTrailEffectComponent) 
	{
		ProjectileTrailEffectComponent->DestroyComponent();
	}
	Super::Destroyed();
}

void AProjectile::TimerDestroyedFinished()
{
	Destroy();
}

void AProjectile::ApplyExplodeDamage()
{	//17.1 пересли с Rocket и сделали общим
	// 7.1 получим контроллер
	const APawn* FiringPawn = GetInstigator();
	// 8.1.2 т.к это теперь у клиента также срабатывает урон будет лишь у сервера регистрироваться
	if (FiringPawn && HasAuthority())
	{	
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{	// 7.2 применим урон зависящий в расстояние от взрыва
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				Damage,
				10.f,
				GetActorLocation(),
				DamageInnerRadius,
				DamageOuterRadius,
				1.f,
				UDamageType::StaticClass(),
				TArray<AActor*>(),
				this,
				FiringController);
		}
	}
}











