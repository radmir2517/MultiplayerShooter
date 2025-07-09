// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionDelegates.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MenuSystemCharacter.generated.h"

class FOnlineSessionSearch;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AMenuSystemCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

public:
	AMenuSystemCharacter();

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
public:
	void BeginPlay() override;

	// функция для удаления существующей и создания новой сессии
	UFUNCTION(BlueprintCallable)
	void CreateGameSession();
	// функция которая запустить поиск сессии по параметрам указанные внутри
	UFUNCTION(BlueprintCallable)
	void JoinGameSession();
protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);
	/** Called for looking input */
	void Look(const FInputActionValue& Value);
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// сделаем указатель на OnlineSession, тип взят с OnlineSubsystem->GetSessionInterface()
	IOnlineSessionPtr OnlineInterfaceSession;
	// это указатель в котором будут указаны настройки поиска сессии
	TSharedPtr<FOnlineSessionSearch> SearchSettings;
	
private:
	// это функция будет вызвано делегатом при создании сессии
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	// // это функция будет вызвано делегатом при найденной сессии заданным SearchSettings
	void OnFindSessionComplete(bool bWasSuccessful);
	// функция которая будете вызываться делегатом OnJoinSessionCompleteDelegate;
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	// делегат который OnlineInterfaceSession будет использовать для информирования был ли создана сессия
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	// делегат, который будет возвращать bool = true если сессия найдена
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	//Делегат, который будет возвращать имя сессии и статус соединения (Success, SessionDoesNotExist, AlreadyInSession)
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
};

