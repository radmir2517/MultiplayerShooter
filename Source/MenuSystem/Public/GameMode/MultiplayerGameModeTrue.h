// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MultiplayerGameModeTrue.generated.h"


namespace MatchState
{
	// 3.1 состояние когда матч окончен и идет показывание итогов
	extern MENUSYSTEM_API const FName Cooldown;
}


class AMultiplayerPlayerController;
class AMultiplayerCharacter;
class AMultiplayerGameState;
UCLASS()
class MENUSYSTEM_API AMultiplayerGameModeTrue : public AGameMode
{
	GENERATED_BODY()

public:

	AMultiplayerGameModeTrue();
	
	// выполняется когда кто то заходит в сессию
	virtual void PostLogin(APlayerController* NewPlayer) override;
	// выполняется когда кто-то выходит из сессии
	virtual void Logout(AController* Exiting) override;
	// функция, которая будет срабатывать когда у игрока будет 0 хп
	virtual void PlayerEliminated(AMultiplayerCharacter* ElimmedCharacter, AMultiplayerPlayerController* ElimmedController, AMultiplayerPlayerController* AttackedController);

	//функция отвязывания персонажа от контроллера и потом спавн нового персонажа с тем же контроллером в рандомном PlayerStart
	virtual void RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController);

	// 2.2 для получения в контроллере WarmupTime (время которое игроки не спаунятся в своих телах а летают по уровню.)
	FORCEINLINE float GetWarmupTime() const { return WarmupTime; }
	// 2.3 для получения в контроллере LevelStartingTime(время которое прошло с начало уровня)
	FORCEINLINE float GetLevelStartingTime() const { return LevelStartingTime; }
	// 4.1 Сделаем геттер, чтобы сервер не считал для себя заново а брал уже готовый в AMultiplayerPlayerController::UpdateTimer()
	FORCEINLINE float GetCountDownTime() const { return CountDownTime; }
	
	// 2.1 время игры
	UPROPERTY(EditDefaultsOnly)
	int32 MatchTime = 120;

	// 4.1 время после матча для показа результатов и перезапуска уровня
	UPROPERTY(EditDefaultsOnly)
	int32 CooldownTime = 10;

protected:
	virtual void BeginPlay() override;
	// переопределим функцию получение измененного состояния 
	virtual void OnMatchStateSet() override;
	
private:

	
	// функция проверки WarmupTime > 0 и будет после запускать матч
	void CountDownCheck();
	
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
	
	// 3.2 добавим переменную для таймера в CountDownCheck
	float CountDownTime = 0.f;
	
};




















