// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

class USphereComponent;

UCLASS()
class MENUSYSTEM_API APickup : public AActor
{
	GENERATED_BODY()

public:
	APickup();
	virtual void Tick(float DeltaTime) override;
	// при разрушение это будет после подъема мы будем воспроизовдить звук
	void Destroyed() override;

	// функции для поднятия предмета
	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
protected:
	virtual void BeginPlay() override;
	// мешка для патронов
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> PickupMeshComponent;
	// компонент сцены просто нужен чтобы повыше поднять сетку и OverlaS[here
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> PickupRoot;
	// сфера пересечения для поднятия
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USphereComponent> SphereComponent;
	// звук поднятия при учничтожения предмета
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundBase> PickupSound;

	
};
