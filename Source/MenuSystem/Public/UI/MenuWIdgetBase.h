// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MenuWIdgetBase.generated.h"

class UMultiplayerSessionSubstem;
class UNEWMultiplayerSessionsSubsystem;

UCLASS()
class MENUSYSTEM_API UMenuWIdgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	// при начало игры будет инициализация
	virtual bool Initialize() override;
	
	virtual void NativeDestruct() override;
	// функция вывода виджета на экран
	UFUNCTION(BlueprintCallable, Category="Setup")
	void MenuSetup(const int32 NumOfPublicConnections = 4, const FString TypeOfMatch = "FreeForAll", const FString PathOfLevel = "Lobby");
	// функция в котором будут прописаны все привязки делегатов 
	void BindCallBacksToDependencies();
	
	// указатель кнопки Host, bindWidget сам привязывается к кнопки.
	UPROPERTY(meta=(BindWidget))
	UButton* Button_Host;
	// указатель кнопки Join, bindWidget сам привязывается к кнопки.
	UPROPERTY(meta=(BindWidget))
	UButton* Button_Join;
	// указатель на субсистему
	UPROPERTY()
	UMultiplayerSessionSubstem* MultiplayerSessionsSubsystem;
protected:
	// функция вызываемая нажатая на кнопку Host 
	UFUNCTION()
	void OnHostButtonClicked();
	// функция вызываемая нажатая на кнопку Join 
	UFUNCTION()
	void OnJoinButtonClicked();

private:
	//
	//функции обратных вызовов с делегатов 
	//
	UFUNCTION()
	void OnCreateSessionComplete(const bool bWasSuccessful);
	void OnFindSessionComplete(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful);
	void OnJoinSessionComplete(const EOnJoinSessionCompleteResult::Type Result);
	UFUNCTION()
	void OnStartSessionComplete(bool bWasSuccessful);
	UFUNCTION()
	void OnDeleteSessionComplete (bool bWasSuccessful);

	//
	// end функции обратных вызовов с делегатов 
	//
	// переменные которые будут использоваться только внутри виджета
	int32 NumPublicConnections;
	// будет задаваться в MenuSetup()
	FString MatchType;
	// путь к уровню
	FString PathToLevel;
};
