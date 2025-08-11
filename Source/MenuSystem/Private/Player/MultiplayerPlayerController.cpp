// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/MultiplayerPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Character/MultiplayerCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameMode.h"
#include "GameMode/MultiplayerGameModeTrue.h"
#include "GameState/MultiplayerGameState.h"
#include "HUD/MultiplayerHUD.h"
#include "Kismet/GameplayStatics.h"
#include "MultiplayerComponent/CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/MultiplayerPlayerState.h"
#include "UI/CharacterOverlay.h"


AMultiplayerPlayerController::AMultiplayerPlayerController()
{
	bReplicates = true;
}

void AMultiplayerPlayerController::BeginPlay()
{
	Super::BeginPlay();


	// Add Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InputMappingContext, 0);
	}
	// установка таймера на отчет общего времени матча для игрока
	GetWorldTimerManager().SetTimer(MatchTimerHandle,this,&AMultiplayerPlayerController::UpdateTimer, 1.f,true);
	// запуск функции о выявление в отставании от времени, таймер запускается в ReceivedPlayer
	CheckTimeSync();

	//2.4 получение всех переменных после присоединение к матчку
	ServerCheckMatchState();
	// 1.1 проверим что у нас есть HUD и создадим анносирующий виджет
	MultiplayerHUD = MultiplayerHUD ? MultiplayerHUD : MultiplayerHUD = Cast<AMultiplayerHUD>(GetHUD());
	if (MultiplayerHUD && !AnnouncementWidget)
	{
		AnnouncementWidget = MultiplayerHUD->CreateAnnouncementWidget();
	}
	// 24.12 Привяжемся к делегату который вызовется при создание виджета Overlay и обнвоим значение гранаты
	OnInitializeOverlayInitializedDelegate.BindUObject(this, &AMultiplayerPlayerController::HandleOverlayCreated);
	
}

void AMultiplayerPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AMultiplayerPlayerController,MatchState);
}

void AMultiplayerPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	// получим персонажа и обновим полоску здоровья
	AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(InPawn);
	if (MultiplayerCharacter)
	{
		MultiplayerCharacter->UpdateHUDHealth();
	}
}
// функция которая активируется на сервере когда клиент соединился к серверу, но при этом может не быть пешки еще. Используется на клиенте
void AMultiplayerPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	// запуск таймера о выявление в отставании от времени 
	if (IsLocalController())
	{
		GetWorldTimerManager().SetTimer(SyncTimerHandle,this,&AMultiplayerPlayerController::CheckTimeSync,TimeToSyncTimer,true);
	}
}

void AMultiplayerPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInputComponent)
	{
		EnhancedInputComponent->BindAction(MoveAction,ETriggerEvent::Triggered,this,&AMultiplayerPlayerController::Move);
		EnhancedInputComponent->BindAction(LookAction,ETriggerEvent::Triggered,this,&AMultiplayerPlayerController::Look);
		EnhancedInputComponent->BindAction(JumpAction,ETriggerEvent::Started,this,&AMultiplayerPlayerController::Jump);
		EnhancedInputComponent->BindAction(EAction,ETriggerEvent::Started,this,&AMultiplayerPlayerController::Interaction);
		EnhancedInputComponent->BindAction(CrouchAction,ETriggerEvent::Started,this,&AMultiplayerPlayerController::Crouch);
		EnhancedInputComponent->BindAction(AimAction,ETriggerEvent::Started,this,&AMultiplayerPlayerController::AimPressed);
		EnhancedInputComponent->BindAction(AimAction,ETriggerEvent::Completed,this,&AMultiplayerPlayerController::AimReleased);
		EnhancedInputComponent->BindAction(FireAction,ETriggerEvent::Started,this,&AMultiplayerPlayerController::FireButtonPressed);
		EnhancedInputComponent->BindAction(FireAction,ETriggerEvent::Completed,this,&AMultiplayerPlayerController::FireButtonReleased);
		EnhancedInputComponent->BindAction(ReloadAction,ETriggerEvent::Started,this,&AMultiplayerPlayerController::ReloadButtonPressed);
		EnhancedInputComponent->BindAction(ThrowGrenadeAction,ETriggerEvent::Started,this,&AMultiplayerPlayerController::ThrowGrenadeButtonPressed);
	}
}


void AMultiplayerPlayerController::Move(const FInputActionValue& Value)
{
	// 5.2 выключим управление когда GameState == Cooldown
	if (bDisableGameplay) return;
	
	FVector2D MovementVector = Value.Get<FVector2D>();

	FRotator OwnControlRotation = GetControlRotation();
	FRotator YawRotation = FRotator(0.f, OwnControlRotation.Yaw,0.f);

	FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (APawn* OwnPawn = GetPawn())
	{
		OwnPawn->AddMovementInput(ForwardDirection,MovementVector.Y);
		OwnPawn->AddMovementInput(RightDirection,MovementVector.X);
	}		
}

void AMultiplayerPlayerController::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
		
	// add yaw and pitch input to controller
	AddYawInput(LookAxisVector.X);
	AddPitchInput(LookAxisVector.Y);
}

void AMultiplayerPlayerController::Jump(const FInputActionValue& Value)
{
	// 5.2 выключим управление когда GameState == Cooldown
	if (bDisableGameplay) return;
	
	if (AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetPawn()))
	{
		if (MultiplayerCharacter->bIsCrouched)
		{
			// при нажатия на прыжок мы крались то встанем
			MultiplayerCharacter->UnCrouch();
		}
		else
		{// при нажатия прыжок мы не крались то прыгнем
			MultiplayerCharacter->Jump();
		}
	}
}

void AMultiplayerPlayerController::Interaction(const FInputActionValue& Value)
{
	// 5.2 выключим управление когда GameState == Cooldown
	if (bDisableGameplay) return;
	
	if (AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetPawn()))
	{	// при взаимодействия экипируем оружие
		MultiplayerCharacter->EquipWeapon();
	}
}

void AMultiplayerPlayerController::Crouch(const FInputActionValue& Value)
{
	// 5.2 выключим управление когда GameState == Cooldown
	if (bDisableGameplay) return;
	
	
	if (AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetPawn()))
	{
		if (!MultiplayerCharacter->bIsCrouched)
		{
			// при нажатия на приседания запустим приседания у персонажа
			MultiplayerCharacter->Crouch();
		}
		else
		{// при нажатия на приседания запустим вставания у персонажа
			MultiplayerCharacter->UnCrouch();
		}
	}
}

void AMultiplayerPlayerController::AimPressed(const FInputActionValue& Value)
{
	// 5.2 выключим управление когда GameState == Cooldown
	if (bDisableGameplay) return;
	
	if (AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetPawn()))
	{	// при нажатии ПКМ активируем State machine в MultiplayerAnimInstance
		MultiplayerCharacter->SetIsAiming(true);
	}
}

void AMultiplayerPlayerController::AimReleased(const FInputActionValue& Value)
{
	// 5.2 выключим управление когда GameState == Cooldown
	if (bDisableGameplay) return;
	
	if (AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetPawn()))
	{	// при отжатии ПКМ деактивируем State machine в MultiplayerAnimInstance
		MultiplayerCharacter->SetIsAiming(false);
	}
}

void AMultiplayerPlayerController::FireButtonPressed(const FInputActionValue& Value)
{
	
	// 5.2 выключим управление когда GameState == Cooldown
	if (bDisableGameplay) return;
	
	if (AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetPawn()))
	{	// при отжатии ПКМ деактивируем State machine в MultiplayerAnimInstance
		MultiplayerCharacter->FireButtonPressed(true);
	}
}

void AMultiplayerPlayerController::FireButtonReleased(const FInputActionValue& Value)
{
	
	// 5.2 выключим управление когда GameState == Cooldown
	if (bDisableGameplay) return;
	
	if (AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetPawn()))
	{	// при отжатии ПКМ деактивируем State machine в MultiplayerAnimInstance
		MultiplayerCharacter->FireButtonPressed(false);
	}
}

void AMultiplayerPlayerController::ReloadButtonPressed(const FInputActionValue& Value)
{
	
	// 5.2 выключим управление когда GameState == Cooldown
	if (bDisableGameplay) return;
	
	if (AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetPawn()))
	{	// при нажатии на R активируем перезарядку
		MultiplayerCharacter->GetCombatComponent()->Reload();
	}
}

void AMultiplayerPlayerController::ThrowGrenadeButtonPressed(const FInputActionValue& Value)
{
	// 5.2 выключим управление когда GameState == Cooldown
	if (bDisableGameplay) return;
	
	if (AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetPawn()))
	{	// при нажатии на G активируем бросок
		MultiplayerCharacter->GetCombatComponent()->ThrowGrenade();
	}
}


void AMultiplayerPlayerController::SetHUDHealth(const float Health, const float MaxHealth)
{
	// проверим что у нас есть HUD
	MultiplayerHUD = MultiplayerHUD ? MultiplayerHUD : MultiplayerHUD = Cast<AMultiplayerHUD>(GetHUD());
	
	if (MultiplayerHUD)
	{	// получим виджет оверлея
		UCharacterOverlay* CharacterOverlay = Cast<UCharacterOverlay> (MultiplayerHUD->GetCharacterOverlayWidget());
		if (CharacterOverlay && CharacterOverlay->HealthBar && CharacterOverlay->HealthText)
		{	// превратим в процент
			float HealthPercent = (Health/MaxHealth);
			// назначим в прогресс бар
			CharacterOverlay->HealthBar->SetPercent(HealthPercent);
			// также превратим цифры в текст и назначим в блок текст
			FString HealthText = FString::Printf(TEXT("%i/%i"),  FMath::CeilToInt32(Health), FMath::CeilToInt32(MaxHealth));
			CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
		}
	}
}

void AMultiplayerPlayerController::SetHUDScore(const float Score)
{
	// проверим что у нас есть HUD
	MultiplayerHUD = MultiplayerHUD ? MultiplayerHUD : MultiplayerHUD = Cast<AMultiplayerHUD>(GetHUD());
	
	if (MultiplayerHUD)
	{	// получим виджет оверлея
		UCharacterOverlay* CharacterOverlay = Cast<UCharacterOverlay> (MultiplayerHUD->GetCharacterOverlayWidget());
		if (CharacterOverlay && CharacterOverlay->ScopeAmount)
		{	// превратим значение Scope в текст и назначим виджету
			FString ScopeText = FString::Printf(TEXT("%i"),  FMath::CeilToInt32(Score));
			CharacterOverlay->ScopeAmount->SetText(FText::FromString(ScopeText));
		}
	}
}

void AMultiplayerPlayerController::SetHUDDefeats(int32 Defeats)
{
	// проверим что у нас есть HUD
	MultiplayerHUD = MultiplayerHUD ? MultiplayerHUD : MultiplayerHUD = Cast<AMultiplayerHUD>(GetHUD());
	
	if (MultiplayerHUD)
	{	// получим виджет оверлея
		UCharacterOverlay* CharacterOverlay = Cast<UCharacterOverlay> (MultiplayerHUD->GetCharacterOverlayWidget());
		if (CharacterOverlay && CharacterOverlay->DefeatsAmount)
		{	// превратим значение Defeats в текст и назначим виджету
			FString DefeatsText = FString::Printf(TEXT("%i"), Defeats);
			CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
		}
	}
}

void AMultiplayerPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	// проверим что у нас есть HUD
	MultiplayerHUD = MultiplayerHUD ? MultiplayerHUD : MultiplayerHUD = Cast<AMultiplayerHUD>(GetHUD());
	
	if (MultiplayerHUD)
	{	// получим виджет оверлея
		UCharacterOverlay* CharacterOverlay = Cast<UCharacterOverlay> (MultiplayerHUD->GetCharacterOverlayWidget());
		if (CharacterOverlay && CharacterOverlay->WeaponAmmoAmount)
		{	// превратим значение Ammo в текст и назначим виджету
			FString AmmoText = FString::Printf(TEXT("%i"), Ammo);
			CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
		}
	}
}

void AMultiplayerPlayerController::SetHUDCarriedAmmo(int32 CarriedAmmo)
{
	// проверим что у нас есть HUD
	MultiplayerHUD = MultiplayerHUD ? MultiplayerHUD : MultiplayerHUD = Cast<AMultiplayerHUD>(GetHUD());
	
	if (MultiplayerHUD)
	{	// получим виджет оверлея
		UCharacterOverlay* CharacterOverlay = Cast<UCharacterOverlay> (MultiplayerHUD->GetCharacterOverlayWidget());
		if (CharacterOverlay && CharacterOverlay->CarriedAmmo)
		{	// превратим значение Ammo в текст и назначим виджету
			FString CarriedAmmoText = FString::Printf(TEXT("%i"), CarriedAmmo);
			CharacterOverlay->CarriedAmmo->SetText(FText::FromString(CarriedAmmoText));
		}
	}
}

void AMultiplayerPlayerController::SetHUDMatchCountDown(int32 CountDownTime)
{
	if (!IsLocalController()) return;
	
	int32 Minutes = CountDownTime / 60;
	int32 Seconds = CountDownTime % 60;

	// проверим что у нас есть HUD
	MultiplayerHUD = MultiplayerHUD ? MultiplayerHUD : MultiplayerHUD = Cast<AMultiplayerHUD>(GetHUD());
	
	if (MultiplayerHUD)
	{	// получим виджет оверлея
		UCharacterOverlay* CharacterOverlay = Cast<UCharacterOverlay> (MultiplayerHUD->GetCharacterOverlayWidget());
		
		if (CharacterOverlay && CharacterOverlay->MatchCountdownText)
		{
			// 4.53 Добавим проверку нуля
			if (CountDownTime < 0) CharacterOverlay->MatchCountdownText->SetText(FText());
			
			// превратим значение Ammo в текст и назначим виджету
			FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
			CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
		}
	}
}

void AMultiplayerPlayerController::SetHUDAnnouncementCountdown(int32 CountDownTime)
{
	if (!IsLocalController()) return;
	
	int32 Minutes = CountDownTime / 60;
	int32 Seconds = CountDownTime % 60;

	// проверим что у нас есть HUD
	MultiplayerHUD = MultiplayerHUD ? MultiplayerHUD : MultiplayerHUD = Cast<AMultiplayerHUD>(GetHUD());

	if (!MultiplayerHUD) return;
	
	// получим виджет оверлея
	AnnouncementWidget = MultiplayerHUD->GetAnnouncementWidget();

	if (MatchState == MatchState::WaitingToStart)
	{
		if (AnnouncementWidget && AnnouncementWidget->WarmupTimer)
		{	// 4.61 Добавим проверку нуля
			if (CountDownTime < 0) AnnouncementWidget->WarmupTimer->SetText(FText());
			// превратим значение Ammo в текст и назначим виджету
			FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
			AnnouncementWidget->WarmupTimer->SetText(FText::FromString(CountdownText));
		}
	}
	// 4.4 Добавим проверку нового состояния
	else if (MatchState == MatchState::Cooldown)
	{
		if (AnnouncementWidget && AnnouncementWidget->WarmupTimer && AnnouncementWidget->AnnouncementText && AnnouncementWidget->InfoText)
		{
			// 4.62 Добавим проверку нуля
			if (CountDownTime < 0) AnnouncementWidget->WarmupTimer->SetText(FText());
			// превратим значение Ammo в текст и назначим виджету
			FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
			AnnouncementWidget->WarmupTimer->SetText(FText::FromString(CountdownText));
			// 4.5 изменим AnnouncementText чтобы игрокам было понятно когда начнется нвоый раунд
			FString AnnouncementText = "New Match Starts In:";
			AnnouncementWidget->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			//6.1 получим GameState чтобы получить список топового игрока
			MultiplayerGameState = MultiplayerGameState ? MultiplayerGameState : MultiplayerGameState = Cast<AMultiplayerGameState>(UGameplayStatics::GetGameState(this));
			if (MultiplayerGameState)
			{	// создадим FString где будет написан(ы) игрок
				FString TextForInfo;
				if (MultiplayerGameState->TopScoringPlayers.Num() == 0)
				{	// 6.2 если после таймера нет победителей
					TextForInfo = FString("There is no chicken winner ))");
				}
				else if (MultiplayerGameState->TopScoringPlayers.Num() > 1)
				{	// 6.3 если несколько людей набравших одинаковое число
					TextForInfo = FString::Printf(TEXT("Players tied for the win: \n"));
					for (auto ScopingPlayer : MultiplayerGameState->TopScoringPlayers)
					{
						FString PlayersName = FString::Printf(TEXT("%s \n"), *ScopingPlayer->GetName());
						TextForInfo = TextForInfo.Append(PlayersName);
					}
				}
				else if (MultiplayerGameState->TopScoringPlayers[0] == GetPlayerState<AMultiplayerPlayerState>())
				{	// 6.4 если победитель один и это владелец PlayerController
					TextForInfo = FString::Printf(TEXT("You're winner!!!"));
				}
				else if (MultiplayerGameState->TopScoringPlayers.Num() == 1 )
				{	// 6.5 если победитель один и это другой игрок
					TextForInfo = FString::Printf(TEXT("%s \n"), *MultiplayerGameState->TopScoringPlayers[0]->GetName());
				}
				// 6.5 изменим текст виджета
				AnnouncementWidget->InfoText->SetText(FText::FromString(TextForInfo));
			}
		}
	}
}

void AMultiplayerPlayerController::SetHUDGrenadesAmount(int32 GrenadesAmount)
{
	// 24.2 проверим что у нас есть HUD
	MultiplayerHUD = MultiplayerHUD ? MultiplayerHUD : MultiplayerHUD = Cast<AMultiplayerHUD>(GetHUD());
	
	if (MultiplayerHUD)
	{	// 24.3 получим виджет оверлея
		UCharacterOverlay* CharacterOverlay = Cast<UCharacterOverlay> (MultiplayerHUD->GetCharacterOverlayWidget());
		if (CharacterOverlay && CharacterOverlay->GrenadesAmount)
		{	// 24.4 превратим значение GrenadesAmmo в текст и назначим виджету
			FString GrenadesAmountText = FString::Printf(TEXT("%i"), GrenadesAmount);
			CharacterOverlay->GrenadesAmount->SetText(FText::FromString(GrenadesAmountText));
		}
	}
}

void AMultiplayerPlayerController::UpdateTimer()
{ 
	float RemainingTime = 0.f;
	// 2.4 проверим статусы и будем отнимать время от серверного каждую секунду
	if (MatchState == MatchState::WaitingToStart) RemainingTime =  WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) RemainingTime =  WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	// 4.3 добавим CooldownTime для показа сцены Cooldown
	else if (MatchState == MatchState::Cooldown) RemainingTime =  WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;
	/*
	// будем отнимать время /  так было до провреки статусов
	RemainingTime = MatchTime - (GetWorld()->GetTimeSeconds() + ClientServerDelta);*/

	// 4.7 Серверный игрок будет получать время с Gamemode чтобы не было отставаний
	if (HasAuthority())
	{
		// проверим что у нас есть MultiplayerGameMode
		MultiplayerGameMode = MultiplayerGameMode ? MultiplayerGameMode : MultiplayerGameMode = Cast<AMultiplayerGameModeTrue>(UGameplayStatics::GetGameMode(this));
		if (MultiplayerGameMode)
		{
			RemainingTime = MultiplayerGameMode->GetCountDownTime();
		}
	}
	
	RemainingTime = FMath::CeilToInt(RemainingTime);
	// 2.5 и обновлять HUD в зависимости от статуса
	if (MatchState == MatchState::WaitingToStart) SetHUDAnnouncementCountdown(RemainingTime);
	else if (MatchState == MatchState::InProgress) SetHUDMatchCountDown(RemainingTime);
	// 4.4 изменим текст таймера в AnnouncementCountdown
	else if (MatchState == MatchState::Cooldown) SetHUDAnnouncementCountdown(RemainingTime);
	/*
	if (RemainingTime <= 0 && MatchState == MatchState::Cooldown)
	{	// если стало равно 0 то выключим таймер
		GetWorldTimerManager().ClearTimer(MatchTimerHandle);
		GetWorldTimerManager().ClearTimer(SyncTimerHandle);
	}*/
}

void AMultiplayerPlayerController::ServerCheckMatchState_Implementation()
{
	// 2.1 получем с Gamemode переменные которые будут в расчетах времени
	MultiplayerGameMode = Cast<AMultiplayerGameModeTrue>(UGameplayStatics::GetGameMode(this));
	if (MultiplayerGameMode)
	{
		MatchTime = MultiplayerGameMode->MatchTime;
		WarmupTime = MultiplayerGameMode->GetWarmupTime();
		LevelStartingTime = MultiplayerGameMode->GetLevelStartingTime();
		MatchState = MultiplayerGameMode->GetMatchState();
		CooldownTime = MultiplayerGameMode->CooldownTime;
		// 2.2 назначим эти переменные клиенту разом в начале присоединения
		ClientCheckMatchState(MatchTime, WarmupTime, LevelStartingTime,CooldownTime, MatchState);
	}
}

void AMultiplayerPlayerController::ClientCheckMatchState_Implementation(int32 Match, int32 Warmup, int32 LevelStarting, int32 Cooldown, FName NameOfMatch)
{
	// 2.3 назначим эти переменные клиенту разом в начале присоединения 
	MatchTime = Match;
	WarmupTime = Warmup;
	LevelStartingTime = LevelStarting;
	MatchState = NameOfMatch;
	CooldownTime = Cooldown;
	OnMatchStateSet(MatchState);
}

void AMultiplayerPlayerController::ServerRequestServerTime_Implementation(float ClientTime)
{
	// получим время сервера и запустим на клиенте функцию
	float CurrentServerTime =  GetWorld()->GetTimeSeconds();
	if (MatchState == MatchState::WaitingToStart)
	{
		// 1.1 проверим что у нас есть HUD и создадим анносирующий виджет
		MultiplayerHUD = MultiplayerHUD ? MultiplayerHUD : MultiplayerHUD = Cast<AMultiplayerHUD>(GetHUD());
		if (MultiplayerHUD && !AnnouncementWidget)
		{
			AnnouncementWidget = MultiplayerHUD->CreateAnnouncementWidget();
		}
	}
	ClientRequestServerTime(ClientTime, CurrentServerTime);
}


void AMultiplayerPlayerController::ClientRequestServerTime_Implementation(float ClientTime, float ServerTime)
{
	// получение разницы времени между сервером и клиентом
	float RoundTripTime =  GetWorld()->GetTimeSeconds() - ClientTime;
	// получение настощего времени сервера
	float CurrentServerTime = ServerTime + (0.5 * RoundTripTime);
	// рассчитываемое время клиента
	ClientServerDelta =  CurrentServerTime - GetWorld()->GetTimeSeconds();
	if (MatchState == MatchState::WaitingToStart)
	{
		// 1.1 проверим что у нас есть HUD и создадим анносирующий виджет
		MultiplayerHUD = MultiplayerHUD ? MultiplayerHUD : MultiplayerHUD = Cast<AMultiplayerHUD>(GetHUD());
		if (MultiplayerHUD && !AnnouncementWidget)
		{
			AnnouncementWidget = MultiplayerHUD->CreateAnnouncementWidget();
		}
	}
}

float AMultiplayerPlayerController::GetServerTime()
{
	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void AMultiplayerPlayerController::CheckTimeSync()
{
	ServerRequestServerTime(GetWorld()->GetTimeSeconds());
}

void AMultiplayerPlayerController::OnMatchStateSet(FName InMatchState)
{// при изменения состояния MatchState на сервере изменим переменную созданную вручную в PlayerController
	if (!HasAuthority()) return;
	// изменим переменную, в клиентах сработает OnRep_MatchState();
	MatchState = InMatchState;
	// проверим что он в прогрессе
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	// 3.1 добавим проверку для нового состояния
	else if (MatchState == MatchState::Cooldown)
	{
		HandleMatchCooldown();
	}
}

void AMultiplayerPlayerController::OnRep_MatchState()
{
	// проверим что он в прогрессе
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	// 3.11 добавим проверку для нового состояния
	else if (MatchState == MatchState::Cooldown)
	{
		HandleMatchCooldown();
	}
	
}

void AMultiplayerPlayerController::HandleMatchHasStarted()
{
	// 1.2 если у нас не нулевой виджет анонса, то скроем его, он в конце игры пригодится
	if (AnnouncementWidget)
	{
		AnnouncementWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	MultiplayerHUD = MultiplayerHUD ? MultiplayerHUD : MultiplayerHUD = Cast<AMultiplayerHUD>(GetHUD());
	// чтобы наверняка зададим управление
	FInputModeGameOnly ModeGameOnly;
	SetInputMode(ModeGameOnly);
	// создадим виджет главный
	if (!MultiplayerHUD) return;
	CharacterOverlayWidget = MultiplayerHUD->CreateOverlayWidget();

	if (!CharacterOverlayWidget) return;
	AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetPawn());

	// обновим здоровье и очки, чтобы они были не дефолтные
	if (!MultiplayerCharacter) return;
	MultiplayerCharacter->UpdateHUDHealth();
	MultiplayerPlayerState = GetPlayerState<AMultiplayerPlayerState>();
	if (MultiplayerPlayerState)
	{
		MultiplayerPlayerState->AddScorePoints(0.f);
		MultiplayerPlayerState->AddDefeatPoints(0);
	}
}

void AMultiplayerPlayerController::HandleMatchCooldown()
{
	// 3.2 мы удалим CharacterOverlayWidget и покажем AnnouncementWidget
	MultiplayerHUD = MultiplayerHUD ? MultiplayerHUD : MultiplayerHUD = Cast<AMultiplayerHUD>(GetHUD());
	if (AnnouncementWidget && MultiplayerHUD && CharacterOverlayWidget)
	{
		CharacterOverlayWidget->RemoveFromParent();
		AnnouncementWidget->SetVisibility(ESlateVisibility::Visible);
	}
	// 5.1 выключим некоторое управление и поворот персонажа булевой
	bDisableGameplay = true;
	// 5.3 сделаем кнопку отпущенной чтобы если была до этого активированна перезарядка то перестал стрелять
	if (AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetPawn()))
	{	// при отжатии ПКМ деактивируем State machine в MultiplayerAnimInstance
		MultiplayerCharacter->FireButtonPressed(false);
		// отключим поворот туловищем
		MultiplayerCharacter->bUseControllerRotationYaw = false;
	}
}

void AMultiplayerPlayerController::HandleOverlayCreated(bool bOverlayCreated)
{
	// 24.13 Когда сработает делегат OnInitializeOverlayInitializedDelegate мы обновим значение патронов
	AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetPawn());
	if (bOverlayCreated && MultiplayerCharacter)
	{
		SetHUDGrenadesAmount(MultiplayerCharacter->GetCombatComponent()->GetGrenadesAmount());
	}
}



















