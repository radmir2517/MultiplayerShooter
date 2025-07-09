// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MultiplayerPlayerState.generated.h"

class AMultiplayerCharacter;
class AMultiplayerPlayerController;
/**
 * 
 */
UCLASS()
class MENUSYSTEM_API AMultiplayerPlayerState : public APlayerState
{
	GENERATED_BODY()
public:

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	// переопределим функцию чтобы еще обновлялось HUD и очки в нем
	virtual void OnRep_Score() override;
	// функция добавление очков игроку и обновление в виджете
	void AddScorePoints(float Points);
	// функция добавление Defeats игроку и обновление в виджете
	void AddDefeatPoints(int32 Points);
	
private:
	UPROPERTY(ReplicatedUsing=OnRep_Defeats)
	int32 Defeats;

	UFUNCTION()
	void OnRep_Defeats();
	
	UPROPERTY()
	AMultiplayerCharacter* MultiplayerCharacter;
	UPROPERTY()
	AMultiplayerPlayerController* MultiplayerController;
};
