// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/PickupSpawnPoint.h"
#include "Pickups/Pickup.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	// запустим спавна таймер спавна объекта
	if (HasAuthority())
	{
		StartPickupSpawnTimer(nullptr);
	}
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickupSpawnPoint::SpawnPickup()
{	// 30.4.1 Проверим что массив не пустой
	if (PickupTypesForSpawn.Num() > 0)
	{
		// 30.4.2 отменим привязку на старый объект если он был
		if (PickupActor)
		{
			PickupActor->OnDestroyed.RemoveAll(this);
		}
		// 30.4.3 Возьмем индекс рандомного предмета из массива
		int32 MaxIndexInArray = PickupTypesForSpawn.Num() - 1;
		int32 RandomIndex = FMath::RandRange(0, MaxIndexInArray);
		// 30.4.4 завспавним объект
		PickupActor = GetWorld()->SpawnActorDeferred<APickup>(PickupTypesForSpawn[RandomIndex],GetActorTransform());
		// 30.4.5 Проверим что он не пустой и привяжемся к его делегату уничтожения чтобы вызвать снова спавн таймер
		if (HasAuthority() && PickupActor != nullptr)
		{
			PickupActor->OnDestroyed.AddDynamic(this,&APickupSpawnPoint::StartPickupSpawnTimer);
			PickupActor->FinishSpawning(GetActorTransform());
		}
	}
}

void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	SpawnPickup();
}

void APickupSpawnPoint::StartPickupSpawnTimer(AActor* Actor)
{	// 30.6.1 вычислим рандомное число от минимального до максимального
	float RandomTimeSpawn = FMath::RandRange(MinTimeToSpawn, MaxTimeToSpawn);
	// 30.6.2 установим таймер на спавн предмета
	GetWorldTimerManager().SetTimer(SpawnPickupTimer,this,&APickupSpawnPoint::SpawnPickupTimerFinished,RandomTimeSpawn);
}





























