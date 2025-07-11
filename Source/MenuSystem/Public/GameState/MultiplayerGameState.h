// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MultiplayerGameState.generated.h"

class AMultiplayerPlayerState;
/**
 * 
 */
UCLASS()
class MENUSYSTEM_API AMultiplayerGameState : public AGameState
{
	GENERATED_BODY()

public:
	// 6.3 добавим репликацию для массива 
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// 6.4 добавим функцию которая будет обновлять список топового игрока
	void UpdateTopScore(AMultiplayerPlayerState* ScoringPlayer);

	// 6.1 добавим массив с игроками которые в топе, точнее там обычно будт один самый топовый
	UPROPERTY(Replicated)
	TArray<AMultiplayerPlayerState*> TopScoringPlayers;
	// 6.2 кол-во очков у самого топового игрока
	UPROPERTY()
	float TopPlayerScore;
};



































