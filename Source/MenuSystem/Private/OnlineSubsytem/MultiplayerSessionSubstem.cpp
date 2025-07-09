// Fill out your copyright notice in the Description page of Project Settings.


#include "OnlineSubsytem/MultiplayerSessionSubstem.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"

UMultiplayerSessionSubstem::UMultiplayerSessionSubstem():
// создадим делегаты
OnCreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &UMultiplayerSessionSubstem::OnCreateSessionComplete)),
OnFindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this,&UMultiplayerSessionSubstem::OnFindSessionComplete)),
OnJoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this,&UMultiplayerSessionSubstem::OnJoinSessionComplete)),
OnStartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this,&UMultiplayerSessionSubstem::OnStartSessionComplete)),
OnDestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UMultiplayerSessionSubstem::OnDestroySessionComplete))
{	
	
}

void UMultiplayerSessionSubstem::CreateSession(const int32 NumPublicConnections,const FString& InMatchType)
{
	// получим субсистему и интрефейс из него
	OnlineSubsystem = Online::GetSubsystem(GEngine->GetWorldFromContextObject(GetWorld(), EGetWorldErrorMode::LogAndReturnNull));
	if(OnlineSubsystem)
	{
		OnlineInterfaceSession = OnlineSubsystem->GetSessionInterface();
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Cyan,"OnlineSubsystem->GetSessionInterface()");
			GEngine->AddOnScreenDebugMessage(-1,10.f,FColor::Red,FString::Printf(TEXT("Online Subsystem: %s"),*OnlineSubsystem->GetSubsystemName().ToString()));
		}
	}
	
	if (OnlineInterfaceSession.IsValid())
	{	// если уже существует сессия, то удалим ее
		if (OnlineInterfaceSession->GetNamedSession(NAME_GameSession))
		{
			LastNumPublicConnections = NumPublicConnections;
			LastInMatchType = InMatchType;
			bDestroyOnCreateSession = true;
			
			DestroySession();
			return;
		}
		
		OnCreateSessionCompleteDelegateHandle = OnlineInterfaceSession->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
		// создадим настройки сессии
		SessionSettings = MakeShareable(new FOnlineSessionSettings());
		SessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
		SessionSettings->NumPublicConnections = NumPublicConnections;
		SessionSettings->bAllowJoinInProgress = true;
		SessionSettings->bAllowJoinViaPresence = true;
		SessionSettings->bShouldAdvertise = true;
		SessionSettings->bUsesPresence = true;
		SessionSettings->BuildUniqueId = true;
		SessionSettings->Set(FName("MatchType"), InMatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		
		// важная строка без него не запуститься
		SessionSettings->bUseLobbiesIfAvailable = true;
		
		// создадим сессию
		ULocalPlayer* LocalPlayer =  GetWorld()->GetFirstLocalPlayerFromController();
		if (!OnlineInterfaceSession->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(),NAME_GameSession,*SessionSettings))
		{
			OnlineInterfaceSession->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
			MultiplayerOnCreateSessionCompleteDelegate.Broadcast(false);
		}
	}
}

void UMultiplayerSessionSubstem::FindSession(const int32 MaxSearchResults)
{	// получим субсистему и интерфейс
	OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if(OnlineSubsystem)
	{
		OnlineInterfaceSession = OnlineSubsystem->GetSessionInterface();
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1,10.f,FColor::Red,FString::Printf(TEXT("Online Subsystem: %s"),*OnlineSubsystem->GetSubsystemName().ToString()));
		}
	}
	if (OnlineInterfaceSession.IsValid() && OnlineSubsystem)
	{
		// создаем настройки и запускаем поиск
		SearchSettings = MakeShareable(new FOnlineSessionSearch());
		SearchSettings->MaxSearchResults = MaxSearchResults;
		SearchSettings->bIsLanQuery = OnlineSubsystem->GetSubsystemName() == "NULL" ? true : false;
		SearchSettings->QuerySettings.Set(SEARCH_PRESENCE,true,EOnlineComparisonOp::Equals);
		// добавим делегат в пул делегатов в OnlineInterfaceSession
		OnFindSessionsCompleteDelegateHandle = OnlineInterfaceSession->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);
		ULocalPlayer* LocalPlayer =  GetWorld()->GetFirstLocalPlayerFromController();
		if (!OnlineInterfaceSession->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(),SearchSettings.ToSharedRef()))
		{// если не успешное создание, то очистим пулл от нашего делегата и отправим через свой делегат в меню, что не успешно.
			OnlineInterfaceSession->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
			MultiplayerOnFindSessionCompleteDelegate.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Red,FString::Printf(TEXT("Failed FindSessions")));
			}
		}
	}
}

void UMultiplayerSessionSubstem::JoinSession(FOnlineSessionSearchResult& DesiredSession)
{
	if (!OnlineInterfaceSession.IsValid())
	{
		MultiplayerOnJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Red,FString::Printf(TEXT("UMultiplayerSessionSubstem::JoinSession  !OnlineInterfaceSession.IsValid() ")));
		}
	}
	// добавляем в пулл нашего делегата
	OnJoinSessionCompleteDelegateHandle = OnlineInterfaceSession->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

	ULocalPlayer* LocalPlayer =  GetWorld()->GetFirstLocalPlayerFromController();
	// присоединаяемя к сессии и проверяем был ли он успешен
	if (!OnlineInterfaceSession->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(),NAME_GameSession,DesiredSession))
	{// если не успешно, то чистим пул от нашего делегата и отправляем в виджет результат ошибки
		OnlineInterfaceSession->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		MultiplayerOnJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Red,FString::Printf(TEXT("Failed JoinSession")));
		}
	}
}

void UMultiplayerSessionSubstem::StartSession()
{
}

void UMultiplayerSessionSubstem::DestroySession()
{
	if (!OnlineInterfaceSession.IsValid())
	{
		MultiplayerOnDeleteSessionCompleteDelegate.Broadcast(false);
		return;
	}
	OnDestroySessionCompleteDelegateHandle =  OnlineInterfaceSession->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);

	OnlineInterfaceSession->DestroySession(NAME_GameSession);
}

void UMultiplayerSessionSubstem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{	// очищаем пул от нашего делегата и отправялем состояние успешности в виджет
	if (OnlineInterfaceSession)
	{
		OnlineInterfaceSession->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
	}
	MultiplayerOnCreateSessionCompleteDelegate.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionSubstem::OnFindSessionComplete(bool bWasSuccessful)
{
	if (OnlineInterfaceSession.IsValid())
	{	// если не успешное создание, то очистим пулл от нашего делегата 
		OnlineInterfaceSession->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
		if (SearchSettings->SearchResults.IsEmpty())
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Blue,FString::Printf(TEXT("SearchResults.Num() == 0")));
			}
			MultiplayerOnFindSessionCompleteDelegate.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
			return;
		}
		// и отправим в виджет результаты поиска и успешность поиска
		MultiplayerOnFindSessionCompleteDelegate.Broadcast(SearchSettings->SearchResults, bWasSuccessful);
	}
}

void UMultiplayerSessionSubstem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (OnlineInterfaceSession.IsValid())
	{	// убираем делегат и отправляем результат в виджет
		OnlineInterfaceSession->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
	}
	MultiplayerOnJoinSessionCompleteDelegate.Broadcast(Result);
}

void UMultiplayerSessionSubstem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}

void UMultiplayerSessionSubstem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (!OnlineInterfaceSession.IsValid())
	{	// Если не valid OnlineInterfaceSession, то отправим едлегатом false в меню
		MultiplayerOnDeleteSessionCompleteDelegate.Broadcast(false);
		return;
	}
	// очистим пул от нашего делегата
	OnlineInterfaceSession->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
	if (bDestroyOnCreateSession && bWasSuccessful)
	{	// bDestroyOnCreateSession = true, то снова создадим сессиб
		bDestroyOnCreateSession = false;
		CreateSession(LastNumPublicConnections,LastInMatchType);
	}
	// отправим в меню от успешности
	MultiplayerOnDeleteSessionCompleteDelegate.Broadcast(bWasSuccessful);
}