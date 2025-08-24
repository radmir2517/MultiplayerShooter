// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

class APickup;

UCLASS()
class MENUSYSTEM_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	APickupSpawnPoint();

	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;
	//30.4 Функция рандомного спавна актера из массива
	void SpawnPickup();
	//30.5 Функция срабатываемая при окончания таймера
	void SpawnPickupTimerFinished();
	//30.6 Функция включения спавна таймера. Включается в начале игры и после уничтожения
	UFUNCTION()
	void StartPickupSpawnTimer(AActor* Actor);

private:
	
	FTimerHandle SpawnPickupTimer;
	//30.1  Масссив с классами для спавна
	UPROPERTY(EditDefaultsOnly, Category="SpawnPickup")
	TArray<TSubclassOf<APickup>> PickupTypesForSpawn;
	//30.2 минимальное время спавна
	UPROPERTY(EditDefaultsOnly, Category="SpawnPickup")
	float MinTimeToSpawn = 3.f;
	//30.3 максимальное время спавна
	UPROPERTY(EditDefaultsOnly, Category="SpawnPickup")
	float MaxTimeToSpawn = 8.f;
	// 30.4.0 указатель на актера который заспавниться
	UPROPERTY()
	TObjectPtr<APickup> PickupActor;
};
