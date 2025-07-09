// Copyright Epic Games, Inc. All Rights Reserved.

#include "MenuSystemCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Online/OnlineSessionNames.h"


DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AMenuSystemCharacter

AMenuSystemCharacter::AMenuSystemCharacter():
// CreateUObject позволяет создавать делегат, и назначать вызов его на функцию, также есть CreateLambda если нужна лямбда, а не функция
OnCreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this,&ThisClass::OnCreateSessionComplete)),
OnFindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionComplete)),
OnJoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
	//OnlineInterfaceSession->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);
	//OnlineInterfaceSession->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMenuSystemCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AMenuSystemCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMenuSystemCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMenuSystemCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMenuSystemCharacter::CreateGameSession()
{
	// проверим что валиден указатель на OnlineInterfaceSession
	if (!OnlineInterfaceSession.IsValid()) return;
	// проверим что сессия уже не создана, если создана удалим ее
	FNamedOnlineSession* NamedOnlineSession = OnlineInterfaceSession->GetNamedSession(NAME_GameSession);
	if (NamedOnlineSession)
	{
		OnlineInterfaceSession->DestroySession(NAME_GameSession);
	}
	
	// добавим в список делегатов в OnlineInterfaceSession, чтобы интерфейс потом пробегался по ним и запускал их
	OnlineInterfaceSession->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
	
	// для создания сессия нам нужно получить netId игрока, создать настройки сессии и задать его имя
	ULocalPlayer* Player = GetWorld()->GetFirstLocalPlayerFromController();
	TSharedPtr<FOnlineSessionSettings> OnlineSessionSettings = MakeShareable(new FOnlineSessionSettings());
	// присоединение во время игры присутствие какое=то
	OnlineSessionSettings->bAllowJoinInProgress = true;
	OnlineSessionSettings->bAllowJoinViaPresence = true;
	OnlineSessionSettings->NumPublicConnections = 4;
	OnlineSessionSettings->bUseLobbiesIfAvailable = true;
	
	// добавим параметр в SessionSettings чтобы потом проверять его клиентом, чтобы находить правильную сессию
	FString MatchType = FString("FREE_ALL");
	OnlineSessionSettings->Set(FName("MATCH_TYPE"),MatchType,EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	
	// сделаем не лан, т.е будет через Steam
	OnlineSessionSettings->bIsLANMatch = false;
	// должен ли показывать уведомления Steam
	OnlineSessionSettings->bShouldAdvertise = true;
	// для поиска сеанса который происходит в нашем регионе
	OnlineSessionSettings->bUsesPresence = true;
	// звездочка перед NetId переопределена внутри там класса, она будет возвращать число
	OnlineInterfaceSession->CreateSession(*Player->GetUniqueNetIdForPlatformUser(),NAME_GameSession,*OnlineSessionSettings);
}

void AMenuSystemCharacter::JoinGameSession()
{	
	if (OnlineInterfaceSession.IsValid() && GetWorld())
	{	// добавим делегат в пул делегатов по поиску сесии, чтобы он выполнялся
		
		// настройки поиска сессии
		SearchSettings = MakeShareable(new FOnlineSessionSearch());
		// 10000 т.к по этому идентификатору будут много игр
		SearchSettings->MaxSearchResults = 10000;
		// это не локально поэтому false
		SearchSettings->bIsLanQuery = false;
		//Поиск только по сеансам присутствия
		SearchSettings->QuerySettings.Set(SEARCH_PRESENCE,true,EOnlineComparisonOp::Equals);
		ULocalPlayer* Player = GetWorld()->GetFirstLocalPlayerFromController();
		// запустим функцию поиска сессии по параметрам SearchSettings
		OnlineInterfaceSession->FindSessions(*Player->GetPreferredUniqueNetId(),SearchSettings.ToSharedRef());
	}
}

void AMenuSystemCharacter::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{	// проверим, что сессия создана и выведем сообщение
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1,10.f,FColor::Blue,FString::Printf(TEXT("Session created:%s"),*SessionName.ToString()));
		}
		UGameplayStatics::OpenLevel(this,"Lobby",true,"listen");
		//GetWorld()->ServerTravel("/Game/ThirdPerson/Maps/Lobby?listen",true);
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1,10.f,FColor::Red,FString::Printf(TEXT("Failed to Create Session")));
		}
	}
}

void AMenuSystemCharacter::OnFindSessionComplete(bool bWasSuccessful)
{
	if (bWasSuccessful && SearchSettings.IsValid())
	{// если сессия найдена выведем игрока владельца сессии
		for (auto Result : SearchSettings->SearchResults)
		{
			if (GEngine)
			{
				FString OwningUserId =	*Result.Session.OwningUserId->ToString();
				FString OwningUserName = *Result.Session.OwningUserName;
				GEngine->AddOnScreenDebugMessage(-1,10.f,FColor::Cyan,FString::Printf(TEXT("UserId: %s, UserName: %s"),*OwningUserId, *OwningUserName));
			}
			// получим значения Mach_Type
			FString MatchType;
			Result.Session.SessionSettings.Get(FName("MATCH_TYPE"),MatchType);
			GEngine->AddOnScreenDebugMessage(-1,10.f,FColor::Cyan,FString::Printf(TEXT("MatchType %s"),*MatchType));
			// и если она равно заданной то присоединяемся к сессии
			if (MatchType == FString("FREE_ALL"))
			{
				OnlineInterfaceSession->JoinSession(*GetWorld()->GetFirstLocalPlayerFromController()->GetPreferredUniqueNetId(),NAME_GameSession,Result);
			}
		}
	}
}

void AMenuSystemCharacter::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (OnlineInterfaceSession.IsValid())
	{	// получим адрес, где находиться серве и присоединимся к нему
		FString Address;
		if (OnlineInterfaceSession->GetResolvedConnectString(NAME_GameSession,Address))
		{	// по адресу отправим к серверу
			GetGameInstance()->GetFirstLocalPlayerController()->ClientTravel(Address,TRAVEL_Absolute);
			GEngine->AddOnScreenDebugMessage(-1,10.f,FColor::Cyan,FString::Printf(TEXT("Client Travel Complete, Address: %s"), *Address));
		}
	}
}

void AMenuSystemCharacter::BeginPlay() 
{
	Super::BeginPlay();
	// получим Online подсистему
	const IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{// получим интерфейс системы
		OnlineInterfaceSession = OnlineSubsystem->GetSessionInterface();
	}

}

void AMenuSystemCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AMenuSystemCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
