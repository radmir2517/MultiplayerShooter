// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MultiplayerSessionSubstem.generated.h"


class  IOnlineSubsystem;
// делегат который будет передаваться в MenuWidget об успешности создания сессии
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionCompleteSignature,bool,bWasSuccessful);
// делегат который будет передаваться в MenuWidget об успешности найденной сессии
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionCompleteSignature,const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful);
// делегат который будет передаваться в MenuWidget об успешности присоединения к сессии
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionCompleteSignature,const EOnJoinSessionCompleteResult::Type Result);
// делегат который будет передавать в MenuWidget об успешности старта сессии
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionCompleteSignature,bool,bWasSuccessful);
// делегат который будет передавать в MenuWidget об успешности окончании сессии
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnEndSessionCompleteSignature,bool,bWasSuccessful);

UCLASS()
class MENUSYSTEM_API UMultiplayerSessionSubstem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	// сделаем конструктор где мы получим OnlineSubsystem и OnlineInterfaceSession
	UMultiplayerSessionSubstem();
	// функции который мы будем запускать из вне
	UFUNCTION()
	void CreateSession(const int32 NumPublicConnections, const FString& InMatchType);
	UFUNCTION()
	void FindSession(const int32 MaxSearchResults);
	void JoinSession(FOnlineSessionSearchResult& DesiredSession);
	UFUNCTION()
	void StartSession();
	UFUNCTION()
	void DestroySession();

	//
	// Экземпляры Делегаты
	//
	// экземпляр делегата, который будет передаваться в MenuWidget об успешности создания сессии
	UPROPERTY()
	FMultiplayerOnCreateSessionCompleteSignature MultiplayerOnCreateSessionCompleteDelegate;
	// экземпляр делегата, который будет передаваться в MenuWidget об успешности создания сессии
	FMultiplayerOnFindSessionCompleteSignature MultiplayerOnFindSessionCompleteDelegate;
	// экземпляр делегата, который будет передаваться в MenuWidget об успешности присоединения к сессии
	FMultiplayerOnJoinSessionCompleteSignature MultiplayerOnJoinSessionCompleteDelegate;
	// экземпляр делегата, который будет передавать в MenuWidget об успешности старта сессии
	UPROPERTY()
	FMultiplayerOnStartSessionCompleteSignature MultiplayerOnStartSessionCompleteDelegate;
	// экземпляр делегата, который будет передавать в MenuWidget об успешности окончании сессии
	UPROPERTY()
	FMultiplayerOnEndSessionCompleteSignature MultiplayerOnDeleteSessionCompleteDelegate;
	
private:
	// внутренние функции запускаемые делегатом
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	// сделаем делегаты для добавления их в OnlineInterface и также Handle, чтобы потом можно было управлять их привязкой
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	FDelegateHandle OnCreateSessionCompleteDelegateHandle;
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;
	FOnStartSessionCompleteDelegate OnStartSessionCompleteDelegate;
	FDelegateHandle OnStartSessionCompleteDelegateHandle;
	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;
	
	// указатель на субсистему
	IOnlineSubsystem* OnlineSubsystem = nullptr;
	// указатель на интерфейс сессии
	IOnlineSessionPtr OnlineInterfaceSession = nullptr;

	TSharedPtr<FOnlineSessionSettings> SessionSettings;
	TSharedPtr<FOnlineSessionSearch> SearchSettings;

	FString MatchType = "FreeForAll";
	
	bool bDestroyOnCreateSession = false;
	int32 LastNumPublicConnections;
	FString LastInMatchType;
};
