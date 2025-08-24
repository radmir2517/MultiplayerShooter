// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/AnnouncementWidget.h"
#include "MultiplayerPlayerController.generated.h"

class AMultiplayerGameState;
class AMultiplayerGameModeTrue;
class AMultiplayerPlayerState;
class AMultiplayerHUD;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;

DECLARE_DELEGATE_OneParam(FOverlayInitializedSignature, bool bOverlayCreated);
/**
 * 
 */
UCLASS()
class MENUSYSTEM_API AMultiplayerPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AMultiplayerPlayerController();
	virtual void BeginPlay() override;


	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	// функция которая активируется когда получает пешку во владения, после смерти или в перевый раз например, вызывается гарантированно на сервере, на клиенте в OnRep_Pawn
	virtual void OnPossess(APawn* InPawn) override;
	// функция которая активируется на сервере когда клиент соединился к серверу, но при этом может не быть пешки еще. Используется на клиенте
	virtual void ReceivedPlayer() override;
	
	virtual void SetupInputComponent() override;
	// функция получения виджета и назначения здоровья в прогресс бар и текст
	bool SetHUDHealth(const float Health, const float MaxHealth);
	// функция назначение виджету значения Scope
	void SetHUDScore(const float Score);
	// функция назначение виджету значения Defeats
	void SetHUDDefeats(int32 Defeats);
	// функция назначение виджету значения WeaponAmmo
	bool SetHUDWeaponAmmo(int32 Ammo);
	// функция назначение виджету значения CarriedAmmo
	bool SetHUDCarriedAmmo(int32 CarriedAmmo);
	// функция назначение виджету значения Таймер Матча
	void SetHUDMatchCountDown(int32 CountDownTime);
	// функция назначение виджету значения Таймер Матча
	void SetHUDAnnouncementCountdown(int32 CountDownTime);
	// функция назначение виджету значения Grenades
	void SetHUDGrenadesAmount(int32 GrenadesAmount);
	// 27,2  функция получения виджета и назначения щита в прогресс бар и текст
	bool SetHUDShield(const float Shield, const float MaxShield);
	
	// при изменения состояния MatchState на сервере изменим переменную созданную вручную в PlayerController
	void OnMatchStateSet(FName InMatchState);
	

	// 5.2 сделай геттер для Character для выключения AimOffset
	FORCEINLINE bool IsDisableGameplay() const { return bDisableGameplay; }
	// 6.1 добавим геттер для MatchState для Character в Destroyed
	FORCEINLINE FName GetMatchState() const { return MatchState; }

	// 24.11 Делегат который сообщит когда Overlay будет готов
	FOverlayInitializedSignature OnInitializeOverlayInitializedDelegate;
	
protected:

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump(const FInputActionValue& Value);
	// функция взаимодействия на кнопку E
	void Interaction(const FInputActionValue& Value);
	void Crouch(const FInputActionValue& Value);
	void AimPressed(const FInputActionValue& Value);
	void AimReleased(const FInputActionValue& Value);
	void FireButtonPressed(const FInputActionValue& Value);
	void FireButtonReleased(const FInputActionValue& Value);
	void ReloadButtonPressed(const FInputActionValue& Value);
	void ThrowGrenadeButtonPressed(const FInputActionValue& Value);
	// функция отсчета общего времени матча
	void UpdateTimer();
	// 2.1  функция получение переменных времени и статуса с сервера
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();
	// 2.2 передача переменных времени и статуса с сервера на клиент
	UFUNCTION(Client, Reliable)
	void ClientCheckMatchState(int32 Match, int32 Warmup, int32 LevelStarting, int32 Cooldown, FName NameOfMatch);
	
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputMappingContext> InputMappingContext;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> EAction;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> AimAction;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> FireAction;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> ReloadAction;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> ThrowGrenadeAction;

	UPROPERTY();
	TObjectPtr<AMultiplayerHUD> MultiplayerHUD;

	// таймер отсчета матча
	FTimerHandle MatchTimerHandle;

	/*
	 * Синхронизация времени игры на сервере и на клиенте
	 */
	// получение времени на сервере
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float ClientTime);
	// получение времени сервера + разница между выполнение сервера и клиента
	UFUNCTION(Client, Reliable)
	void ClientRequestServerTime(float ClientTime, float ServerTime);
	// получение времени на сервере минус разница во времени
	float GetServerTime();
	// для таймера, запуск ServerRequestServerTime
	void CheckTimeSync();
	

	// таймер который какждый TimeToSyncTimer выполняет ServerRequestServerTime
	FTimerHandle SyncTimerHandle;
	// разница во временим между клиетом и сервером
	float ClientServerDelta = 0.f;
	// период каждой синхронизации времени между клиентов и сервером
	float TimeToSyncTimer = 5.f;

	// состояние матча
	UPROPERTY(ReplicatedUsing="OnRep_MatchState")
	FName MatchState;
	// виджет
	UPROPERTY()
	TObjectPtr<UUserWidget> CharacterOverlayWidget;
	// состояние матча реплицированное
	UFUNCTION()
	void OnRep_MatchState();
	// функция когда состояние == inprogress
	void HandleMatchHasStarted();
	// 3.1 функция когда состояние == cooldown
	void HandleMatchCooldown();
	// 24. После создания виджета в HUD мы обновим значение гранат
	void HandleOverlayCreated(bool bOverlayCreated);
private:
	// 2.1 время игры
	int32 MatchTime = 0;
	int32 WarmupTime = 0;
	int32 LevelStartingTime = 0;
	// 4.1 время после матча для показа результатов и перезапуска уровня, получим с GameMode
	int32 CooldownTime = 0;

	AMultiplayerPlayerState* MultiplayerPlayerState;
	//1.0 ссылка на анонсирующий виджет, который мы покажем в начале игры и скроем после начала
	UPROPERTY()
	TObjectPtr<UAnnouncementWidget> AnnouncementWidget;
	// 4.2 переменная используемая в UpdateTimer
	UPROPERTY()
	TObjectPtr<AMultiplayerGameModeTrue> MultiplayerGameMode;

	//5.1 булевая которая будет true при GameState == Cooldown и выключать некоторое управление
	bool bDisableGameplay = false;

	//6.1 добавим Game State для получение списка тового игрока не путать с состоянием MatchState и PlayerState
	UPROPERTY()
	AMultiplayerGameState* MultiplayerGameState;
};

















































