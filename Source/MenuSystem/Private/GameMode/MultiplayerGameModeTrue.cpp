// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/MultiplayerGameModeTrue.h"


#include "EngineUtils.h"
#include "Character/MultiplayerCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "GameState/MultiplayerGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/MultiplayerPlayerController.h"
#include "Player/MultiplayerPlayerState.h"

//3.1 добавим новое состояние
namespace MatchState
{
	// 3.11 состояние когда матч окончен и идет показывание итогов
	const FName Cooldown = FName(TEXT("Cooldown"));
}


AMultiplayerGameModeTrue::AMultiplayerGameModeTrue()
{
	bDelayedStart = true;
}

// выполняется когда кто то заходит в сессию
void AMultiplayerGameModeTrue::PostLogin(APlayerController* NewPlayer)
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
void AMultiplayerGameModeTrue::Logout(AController* Exiting)
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

void AMultiplayerGameModeTrue::PlayerEliminated(AMultiplayerCharacter* ElimmedCharacter,
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

void AMultiplayerGameModeTrue::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
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

void AMultiplayerGameModeTrue::BeginPlay()
{
	Super::BeginPlay();

	// запустим таймер который будет проверять время и запускать матч после 10 сек 
	GetWorldTimerManager().SetTimer(WarmupTimerHandle,this,&AMultiplayerGameModeTrue::CountDownCheck,1.f,true);
	// получим время которое уже успело пройти
	LevelStartingTime = GetWorld()->GetTimeSeconds();
	// увеличим пргревочное время
	WarmupTime = WarmupTime + LevelStartingTime;
	//StartMatch(); // сделано через таймер
}

void AMultiplayerGameModeTrue::OnMatchStateSet()
{
	Super::OnMatchStateSet();
	// при изменения состояния MatchState на сервере изменим переменную созданную вручную в PlayerController
	// возьмем итератор и пройдемся по всем контроллерам которые есть в мире
	for (TActorIterator<AMultiplayerPlayerController> It(GetWorld()); It; ++It)
	{
		AMultiplayerPlayerController* PC = Cast<AMultiplayerPlayerController>(*It);
		if (IsValid(PC))
		{// при изменения состояния MatchState на сервере изменим переменную созданную вручную в PlayerController
			PC->OnMatchStateSet(MatchState);
		}
	}
}

void AMultiplayerGameModeTrue::CountDownCheck()
{
	// 3.2 изменим таймер с Warmup = warmup - 1, на более сложное с проверкой статусов
	if (MatchState == MatchState::WaitingToStart)
	{
		if (CountDownTime >= 0.f)
		{	// будем убавлять прогревочное время
			CountDownTime = WarmupTime - GetWorld()->GetTimeSeconds();
		}
		else
		{// если прогревочное время прошло, то запустим матч
			StartMatch();
			// изменим время для следующего этапа
			CountDownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		if (CountDownTime >= 0.f)
		{	// будем убавлять прогревочное и время матча время и будем проверять время
			CountDownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds();
		}
		else
		{	// поменяем статус
			SetMatchState(MatchState::Cooldown);
			CountDownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds();
		}
	}
	// 4.1 для нового состояния будем прибавлять еще CooldownTime
	else if (MatchState == MatchState::Cooldown)
	{
		if (CountDownTime >= 0.f)
		{	// будем убавлять прогревочное и время матча время и будем проверять время
			CountDownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds();
		}
		else
		{	// 5.1 если время отсчитало 0 то перезапустим игру на сервере
			RestartGame();
		}
	}
}













































