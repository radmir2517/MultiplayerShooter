// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/Pickup.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MenuSystem/MenuSystem.h"


APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	PickupRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PickupRoot"));
	SetRootComponent(PickupRoot);
	
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->SetupAttachment(PickupRoot);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
	SphereComponent->AddWorldOffset(FVector(0.0,0.0,50.f));

	PickupMeshComponent =  CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMeshComponent"));
	PickupMeshComponent->SetupAttachment(SphereComponent);
	PickupMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMeshComponent->SetRelativeScale3D(FVector(3.f));
	PickupMeshComponent->SetRenderCustomDepth(true);
	PickupMeshComponent->CustomDepthStencilValue =  CUSTOM_DEPTH_PINK;

	//23.6 Создадим компонент который будет как мешка у нас анимированная
	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>("HealthPickupComponent");
	PickupEffectComponent->SetupAttachment(GetRootComponent());
}

void APickup::BeginPlay()
{
	Super::BeginPlay();
	// привяжемся к оверлапу
	if (HasAuthority())
	{
		SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &APickup::OnSphereBeginOverlap);
	}
}

void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SphereComponent->AddWorldRotation(FRotator(0.f,45.f * DeltaTime,0));
}

void APickup::Destroyed()
{	// воспроизведем звук
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this,PickupSound,GetActorLocation());
	}
	//23.7 перед уничтожения воспроизведем эффект и в родительской еще и звук будет
	if (PickupEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this,PickupEffect,GetActorLocation(),GetActorRotation());
	}
	Super::Destroyed();
}

void APickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

	Destroy();
}



























