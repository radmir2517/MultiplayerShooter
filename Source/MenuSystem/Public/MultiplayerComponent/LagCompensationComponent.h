// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class AMultiplayerPlayerController;
class AMultiplayerCharacter;

//26.2 Информация о локации, вращения и размеров каждой чати бокса тела
USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxEntent;
	
};
// 26.1 Добавим структуру которая будет хранить время и карту с информации
// о BoxCollision каждой части тела которую мы добавили
USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()
	
	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MENUSYSTEM_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULagCompensationComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
	//26.4.1 Сеттеры для контроллера и персонажа
	FORCEINLINE void SetMultiplayerCharacter(AMultiplayerCharacter* Character) {MultiplayerCharacter = Character;}
	FORCEINLINE void SetMultiplayerPlayerController(AMultiplayerPlayerController* PlayerController) {MultiplayerController = PlayerController;}
	//26.4.2 Геттеры для контроллера и персонажа
	FORCEINLINE AMultiplayerCharacter* GetMultiplayerCharacter() {return MultiplayerCharacter;}
	FORCEINLINE AMultiplayerPlayerController* GetMultiplayerPlayerController() {return MultiplayerController;}
	
	//27.5 Созаддим функцию которая будет показывать визуально как расположены коробки
	void ShowFramePackage(const FFramePackage& FramePackage, const FColor& Color);
	//27.1 Создадим функцию которая будет сохранять позицию boxes частей тела и время
	void SafeFramePackage(FFramePackage& FramePackage);
	
protected:
	virtual void BeginPlay() override;
	// 26.3 Указатели о персонаже и его контроллере
	UPROPERTY()
	AMultiplayerCharacter* MultiplayerCharacter;
	UPROPERTY()
	AMultiplayerPlayerController* MultiplayerController;

};














































