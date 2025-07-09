// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/MultiplayerGameMode.h"

#include "Character/MultiplayerCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "GameState/MultiplayerGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/MultiplayerPlayerController.h"
#include "Player/MultiplayerPlayerState.h"

AMultiplayerGameMode::AMultiplayerGameMode()
{
	//bDelayedStart = true;
}

// выполняется когда кто то заходит в сессию
void AMultiplayerGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	// получим GameState
	MultiplayerGameState = GetGameState<AMultiplayerGameState>();
	// получим имя игрока из playerState
	FString NewPlayerName = NewPlayer->GetPlayerState<APlayerState>()->GetPlayerName();
	if (MultiplayerGameState)
	{	// получим число игроков
		int32 PlayerCount = MultiplayerGameState->PlayerArray.Num();
		if (GEngine)
		{	// вывыдем на экран кол-во игроков и имя игрока
			GEngine->AddOnScreenDebugMessage(1,40.f,FColor::Yellow,FString::Printf(TEXT("Current player Count: %i"), PlayerCount));
			GEngine->AddOnScreenDebugMessage(2,40.f,FColor::Yellow,FString::Printf(TEXT("NewPlayer: %s"), *NewPlayerName));
		}
	}
}
// выполняется когда кто-то выходит из сессии
void AMultiplayerGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	// получим GameState
	MultiplayerGameState = GetGameState<AMultiplayerGameState>();
	// получим имя игрока из playerState
	FString LeavingPlayerName = Exiting->GetPlayerState<APlayerState>()->GetPlayerName();
	if (MultiplayerGameState)
	{// получим число игроков
		int32 PlayerCount = MultiplayerGameState->PlayerArray.Num();
		if (GEngine)
		{// вывыдем на экран кол-во игроков и имя игрока уходящего
			GEngine->AddOnScreenDebugMessage(1,40.f,FColor::Yellow,FString::Printf(TEXT("Current player Count: %i"), PlayerCount - 1));
			GEngine->AddOnScreenDebugMessage(2,40.f,FColor::Yellow,FString::Printf(TEXT("LeavingPlayer: %s"), *LeavingPlayerName));
		}
	}
}

void AMultiplayerGameMode::PlayerEliminated(AMultiplayerCharacter* ElimmedCharacter,
	AMultiplayerPlayerController* ElimmedController, AMultiplayerPlayerController* AttackedController)
{
	if (ElimmedCharacter)
	{ // пока запустим монтаж смерти
		ElimmedCharacter->Elim();
	}
	// чтобы назначить очки полсе убийства игрока надо получить их PlayerState
	AMultiplayerPlayerState* ElimmedMultiplayerPlayerState = ElimmedController ? ElimmedController->GetPlayerState<AMultiplayerPlayerState>() : nullptr;
	AMultiplayerPlayerState* AttackedMultiplayerPlayerState = AttackedController ? AttackedController->GetPlayerState<AMultiplayerPlayerState>() : nullptr;
	// проверим что они есть и то что они равны друг другу, чтобы не бивать себя для очков
	if (AttackedMultiplayerPlayerState && ElimmedMultiplayerPlayerState != AttackedMultiplayerPlayerState)
	{
		AttackedMultiplayerPlayerState->AddScorePoints(1.f);
	}
	// засчитаем очко поражения програвшему
	if (ElimmedMultiplayerPlayerState)
	{
		ElimmedMultiplayerPlayerState->AddDefeatPoints(1);
	}
}

void AMultiplayerGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
{
	if (EliminatedCharacter)
	{	// отсоедим персонажа от контроллера и уничтожим Character
		EliminatedCharacter->Reset();
		EliminatedCharacter->Restart();
		EliminatedCharacter->Destroy();
	}
	if (EliminatedController)
	{	// получим все точки спавна
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(EliminatedController,APlayerStart::StaticClass(),PlayerStarts);
		// получим рандомный номер спавна
		int32 RandNumPlayerStart = FMath::RandRange(0,PlayerStarts.Num() - 1);
		// и заспавнем игрока там
		RestartPlayerAtPlayerStart(EliminatedController,PlayerStarts[RandNumPlayerStart]);
		
		//EnableInput(Cast<APlayerController>(EliminatedController));
	}
}

void AMultiplayerGameMode::BeginPlay()
{
	Super::BeginPlay();

	// запустим таймер который будет проверять время и запускать матч после 10 сек 
	//GetWorldTimerManager().SetTimer(WarmupTimerHandle,this,&AMultiplayerGameMode::WarmupCheck,1.f,true);
	// получим время которое уже успело пройти
	LevelStartingTime = GetWorld()->GetTimeSeconds();
	// увеличим пргревочное время
	WarmupTime = WarmupTime + LevelStartingTime;
	StartMatch();
}

void AMultiplayerGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();
	// при изменения состояния MatchState на сервере изменим переменную созданную вручную в PlayerController
	// возьмем итератор и пройдемся по всем контроллерам которые есть в мире
	for (FConstPlayerControllerIterator ControllerIterator = GetWorld()->GetPlayerControllerIterator(); ControllerIterator; ++ControllerIterator)
	{	
		AMultiplayerPlayerController* PC = Cast<AMultiplayerPlayerController>(*ControllerIterator);
		if (PC)
		{	// при изменения состояния MatchState на сервере изменим переменную созданную вручную в PlayerController
			PC->OnMatchStateSet(MatchState);
		}
	}
}

void AMultiplayerGameMode::WarmupCheck()
{
	if (WarmupTime >= 0.f)
	{	// будем убавлять прогревочное время
		WarmupTime = WarmupTime - 1.f;
	}
	else
	{// если прогревочное время прошло то запустим матч
		StartMatch();
		GetWorldTimerManager().ClearTimer(WarmupTimerHandle);
	}
}












































