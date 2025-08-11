// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/Pickup.h"

#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MenuSystem/MenuSystem.h"


APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;

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
	PickupMeshComponent->SetRenderCustomDepth(true);
	PickupMeshComponent->CustomDepthStencilValue =  CUSTOM_DEPTH_PINK;
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
	PickupMeshComponent->AddWorldRotation(FRotator(0.f,0.0f,45.f * DeltaTime));
}

void APickup::Destroyed()
{	// воспроизведем звук
	UGameplayStatics::PlaySoundAtLocation(this,PickupSound,GetActorLocation());
	Super::Destroyed();
}

void APickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	
}

