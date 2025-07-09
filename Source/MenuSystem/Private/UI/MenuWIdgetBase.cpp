// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MenuWIdgetBase.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSubsytem/MultiplayerSessionSubstem.h"

bool UMenuWIdgetBase::Initialize()
{
	if ( !Super::Initialize())
	{
		return false;
	}
	
	if (Button_Host)
	{
		Button_Host->OnClicked.AddDynamic(this,&ThisClass::OnHostButtonClicked);
	}
	if (Button_Join)
	{
		Button_Join->OnClicked.AddDynamic(this,&ThisClass::OnJoinButtonClicked);
	}
	
	return true;
	
}

void UMenuWIdgetBase::MenuSetup(const int32 NumOfPublicConnections, const FString TypeOfMatch, const FString PathOfLevel)
{
    PathToLevel = PathOfLevel;
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;
	
	if (GetWorld())
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		FInputModeUIOnly InputModeData;
		InputModeData.SetWidgetToFocus(TakeWidget());
		InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputModeData);
	}
	if (GetGameInstance())
	{
		MultiplayerSessionsSubsystem = GetGameInstance()->GetSubsystem<UMultiplayerSessionSubstem>();
	}
	NumPublicConnections = NumOfPublicConnections;
	MatchType = TypeOfMatch;

	// привязка делегатов в виджету
	BindCallBacksToDependencies();
}

void UMenuWIdgetBase::BindCallBacksToDependencies()
{
	if (!MultiplayerSessionsSubsystem) return;
	// привяжемся к связанные с созданию сессии, нахождению, присоединению, старту и окончанию
	MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionCompleteDelegate.AddDynamic(this, &ThisClass::OnCreateSessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnFindSessionCompleteDelegate.AddUObject(this,&ThisClass::OnFindSessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionCompleteDelegate.AddUObject(this,&ThisClass::OnJoinSessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnStartSessionCompleteDelegate.AddDynamic(this,&ThisClass::OnStartSessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnDeleteSessionCompleteDelegate.AddDynamic(this,&ThisClass::OnDeleteSessionComplete);
}

void UMenuWIdgetBase::OnHostButtonClicked()
{	// запустим создание сервера
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections,MatchType);
	}
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Blue,FString::Printf(TEXT("OnHostButtonClicked, MatchType:%s"),*MatchType));
	}
}

void UMenuWIdgetBase::OnJoinButtonClicked()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Blue,"OnJoinButtonClicked");
	}
	// запустим поиск сессии
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->FindSession(10000);
	}
	FInputModeGameOnly Data;
	GetGameInstance()->GetFirstLocalPlayerController()->SetInputMode(Data);
	GetGameInstance()->GetFirstLocalPlayerController()->bShowMouseCursor = false;
	RemoveFromParent();
}

void UMenuWIdgetBase::OnCreateSessionComplete(const bool bWasSuccessful)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,15.f,
			FColor::Blue,FString::Printf(TEXT("OnCreateSessionComplete : %d"),bWasSuccessful));
	}
	// если успех, то отправим на карту со свойством listen
	if (bWasSuccessful)
	{
		UGameplayStatics::OpenLevel(GetWorld(),*PathToLevel,false,TEXT("listen"));
		//GetWorld()->ServerTravel("/Game/ThirdPerson/Maps/ThirdPersonMap?listen");
	}
}

void UMenuWIdgetBase::OnFindSessionComplete(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful)
{

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Blue,
			FString::Printf(TEXT("UMenuWIdgetBase::OnFindSessionComplete:begin MultiplayerSessionsSubsystem: %s bWasSuccessful:%d"),*MultiplayerSessionsSubsystem->GetName(),bWasSuccessful));
	}
	
	if (IsValid(MultiplayerSessionsSubsystem) && bWasSuccessful )
	{// если успешен поиск
		
		for (auto Result : SearchResults)
		{	// то вытаскиваем результат и сравниваем строку, чтобы убедиться, что это наша игра
			FString InMatchType;
			Result.Session.SessionSettings.Get(FName("MatchType"), InMatchType);
			if (InMatchType == MatchType)
			{	// если это наша игра, то запускаем присоединение
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Blue,
						FString::Printf(TEXT("UMenuWIdgetBase::OnFindSessionComplete, InMatchType:%s"), *InMatchType));
				}
				// обязательные строки в 5.5 иначе не сможем присоединиться 
				Result.Session.SessionSettings.bUseLobbiesIfAvailable = true;
				Result.Session.SessionSettings.bUsesPresence = true;
				MultiplayerSessionsSubsystem->JoinSession(Result);
				return;
			}
		}
	}
}

void UMenuWIdgetBase::OnJoinSessionComplete(const EOnJoinSessionCompleteResult::Type Result)
{	// получим OnlineSubsystem
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (!OnlineSubsystem) return;
	if (Result == EOnJoinSessionCompleteResult::UnknownError)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Red,
				FString::Printf(TEXT("EOnJoinSessionCompleteResult::UnknownError")));
		}
		return;
	}
		
	IOnlineSessionPtr OnlineInterfaceSession = OnlineSubsystem->GetSessionInterface();
	if (OnlineInterfaceSession.IsValid())
	{	// получим строку адреса для переходан на уровень
		FString ConnectionInfo;
		OnlineInterfaceSession->GetResolvedConnectString(NAME_GameSession,ConnectionInfo);
			
		APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
		if (PlayerController)
		{	// отправим клиента на уровень
			PlayerController->ClientTravel(ConnectionInfo,TRAVEL_Absolute);
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Blue,
				FString::Printf(TEXT("UMenuWIdgetBase::OnJoinSessionComplete MAP: %s"),*ConnectionInfo));
		}
	}
}

void UMenuWIdgetBase::OnStartSessionComplete(bool bWasSuccessful)
{
	
}

void UMenuWIdgetBase::OnDeleteSessionComplete(bool bWasSuccessful)
{
	
}

void UMenuWIdgetBase::NativeDestruct()
{
	// при уничтожении сделаем управление обычным и уберем курсор
	Super::NativeDestruct();
}


