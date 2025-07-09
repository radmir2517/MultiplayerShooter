// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MultiplayerGameMode.generated.h"


class AMultiplayerPlayerController;
class AMultiplayerCharacter;
class AMultiplayerGameState;

UCLASS()
class MENUSYSTEM_API AMultiplayerGameMode : public AGameMode
{
	GENERATED_BODY()

public:

	AMultiplayerGameMode();
	
	// выполняется когда кто то заходит в сессию
	virtual void PostLogin(APlayerController* NewPlayer) override;
	// выполняется когда кто-то выходит из сессии
	virtual void Logout(AController* Exiting) override;
	// функция, которая будет срабатывать когда у игрока будет 0 хп
	virtual void PlayerEliminated(AMultiplayerCharacter* ElimmedCharacter, AMultiplayerPlayerController* ElimmedController, AMultiplayerPlayerController* AttackedController);

	//функция отвязывания персонажа от контроллера и потом спавн нового персонажа с тем же контроллером в рандомном PlayerStart
	virtual void RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController);

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
	
private:

	
	// функция проверки WarmupTime > 0 и будет после запускать матч
	void WarmupCheck();
	
	// указатель на GameState
	UPROPERTY()
	TObjectPtr<AMultiplayerGameState> MultiplayerGameState = nullptr;

	/*
	 * DelayedStart после 10 сек
	 */
	// таймер который будет отсчитывать время Warmup, в котором будут летать по уровню и после стартовать игру
	FTimerHandle WarmupTimerHandle;
	// время которое игроки не спаунятся в своих телах а летают по уровню.
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 2.f;
	// переменная в котором будет хранится время которое прошло с начало уровня
	float LevelStartingTime = 0.f;

	
};













































