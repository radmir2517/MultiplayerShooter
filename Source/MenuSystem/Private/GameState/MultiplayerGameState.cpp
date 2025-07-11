// Fill out your copyright notice in the Description page of Project Settings.


#include "GameState/MultiplayerGameState.h"

#include "Net/UnrealNetwork.h"
#include "Player/MultiplayerPlayerState.h"

void AMultiplayerGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// 6.1 добавим репликацию для массива 
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMultiplayerGameState, TopScoringPlayers);
}

void AMultiplayerGameState::UpdateTopScore(AMultiplayerPlayerState* ScoringPlayer)
{	// 6.2 проверим, если массив пустой то просто добавим его
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopPlayerScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() == TopPlayerScore)
	{	// 6.3 если топовому игроку очки равны то его тоже добавим в список
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() > TopPlayerScore)
	{	// 6.4 если больше топового то чистим массив добавляем нового
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
}
