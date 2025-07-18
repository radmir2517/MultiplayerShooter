// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileBullet.h"

#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Interface/InteractWithCrosshairsInterface.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AProjectileBullet::AProjectileBullet()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// 9.1 установим компонент 
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	// 9.2 вращение снаряда к его направлению скорости
	ProjectileMovement-> bRotationFollowsVelocity = true;
	ProjectileMovement->SetIsReplicated(true);
	ProjectileMovement-> InitialSpeed = 10000.f;
	ProjectileMovement-> MaxSpeed = 10000.f;
}

// Called when the game starts or when spawned
void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();
	
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	// проверим что есть владелец(игрко пустивший пулю и то что игрок это владелец интерфейса прицела)
	if (!GetOwner() && !OtherActor->Implements<UInteractWithCrosshairsInterface>()) return;
	
	// далее надо получить контроллер
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		AController* CharacterController = OwnerCharacter->Controller;
		if (CharacterController)
		{	// запускае функцию нанесения урона, указываем врага, кол-во урона, контроллер владельца пули, кто причиняет урон и тип урона
			UGameplayStatics::ApplyDamage(OtherActor,Damage,CharacterController,this,UDamageType::StaticClass());
		}
	}
	// это запускаем позже, т.к там Destoy()
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}

// Called every frame
void AProjectileBullet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

