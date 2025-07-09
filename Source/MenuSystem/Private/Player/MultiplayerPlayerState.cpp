// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/MultiplayerPlayerState.h"

#include "Character/MultiplayerCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Player/MultiplayerPlayerController.h"

void AMultiplayerPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMultiplayerPlayerState,Defeats)
}

void AMultiplayerPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	// получим персонажа для получение его контроллера
	MultiplayerCharacter = MultiplayerCharacter == nullptr ? Cast<AMultiplayerCharacter>(GetPawn()) : MultiplayerCharacter;
	if (MultiplayerCharacter)
	{	// получим контроллер и назначим новое число Score в HUD
		MultiplayerController = MultiplayerController == nullptr ? Cast<AMultiplayerPlayerController>(MultiplayerCharacter->GetController()) : MultiplayerController;
		if (MultiplayerController)
		{
			MultiplayerController->SetHUDScore(GetScore());
		}
	}
}

void AMultiplayerPlayerState::AddScorePoints(float Points)
{
	// назначим очки и обновим HUD
	SetScore(GetScore() + Points);
	// получим персонажа для получение его контроллера
	MultiplayerCharacter = MultiplayerCharacter == nullptr ? Cast<AMultiplayerCharacter>(GetPawn()) : MultiplayerCharacter;
	if (MultiplayerCharacter)
	{	// получим контроллер и назначим новое число Score в HUD
		MultiplayerController = MultiplayerController == nullptr ? Cast<AMultiplayerPlayerController>(MultiplayerCharacter->GetController()) : MultiplayerController;
		if (MultiplayerController)
		{
			MultiplayerController->SetHUDScore(GetScore());
		}
	}
}

void AMultiplayerPlayerState::AddDefeatPoints(int32 Points)
{
	// прибавим очки
	Defeats += Points;
	MultiplayerCharacter = MultiplayerCharacter == nullptr ? Cast<AMultiplayerCharacter>(GetPawn()) : MultiplayerCharacter;
	if (MultiplayerCharacter)
	{	// получим контроллер и назначим новое число Defeats в HUD
		MultiplayerController = MultiplayerController == nullptr ? Cast<AMultiplayerPlayerController>(MultiplayerCharacter->GetController()) : MultiplayerController;
		if (MultiplayerController)
		{
			MultiplayerController->SetHUDDefeats(Defeats);
		}
	}
}

void AMultiplayerPlayerState::OnRep_Defeats()
{
	MultiplayerCharacter = MultiplayerCharacter == nullptr ? Cast<AMultiplayerCharacter>(GetPawn()) : MultiplayerCharacter;
	if (MultiplayerCharacter)
	{	// получим контроллер и назначим новое число Defeats в HUD
		MultiplayerController = MultiplayerController == nullptr ? Cast<AMultiplayerPlayerController>(MultiplayerCharacter->GetController()) : MultiplayerController;
		if (MultiplayerController)
		{
			MultiplayerController->SetHUDDefeats(Defeats);
		}
	}
}
