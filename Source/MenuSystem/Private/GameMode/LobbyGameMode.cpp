// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"

#include "GameState/MultiplayerGameState.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	const int32 PlayerNum = GetGameState<AMultiplayerGameState>()->PlayerArray.Num();
	if (PlayerNum == 2)
	{
		bUseSeamlessTravel = true;
		GetWorld()->ServerTravel("Game/ThirdPerson/Maps/MultiplayerMap?listen",true);
	}
}
