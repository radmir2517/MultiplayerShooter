// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"

#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"


AProjectileRocket::AProjectileRocket()
{
	//7.3 создадим сетку для снаряда
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMeshComponent->SetupAttachment(CollisionBox);
	
	PrimaryActorTick.bCanEverTick = true;
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	// 7.1 получим контроллер
	const APawn* FiringPawn = GetInstigator();
	if (FiringPawn)
	{	
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{	// 7.2 применим урон зависящий в расстояние от взрыва
			UGameplayStatics::ApplyRadialDamageWithFalloff(
		this,
		Damage,
		10.f,
		GetActorLocation(),
		200.f,
		500.f,
		1.f,
		UDamageType::StaticClass(),
		TArray<AActor*>(),
		this,
		FiringController);
		}
	}
	
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AProjectileRocket::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

