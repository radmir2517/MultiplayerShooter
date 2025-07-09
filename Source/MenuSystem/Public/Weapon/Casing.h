// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

UCLASS()
class MENUSYSTEM_API ACasing : public AActor
{
	GENERATED_BODY()

public:
	ACasing();

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMesh;
	// величина амплитуды толчка
	UPROPERTY(EditDefaultsOnly)
	float CasingImpulseMagnitude = 6.f;
	// время жизни гильзы, чтобы она исчезала потом
	UPROPERTY(EditDefaultsOnly)
	float LifeSpanValue = 10.f;
};






































