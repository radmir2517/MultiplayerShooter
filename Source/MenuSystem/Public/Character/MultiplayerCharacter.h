// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "Interface/InteractWithCrosshairsInterface.h"
#include "MultiplayerCharacter.generated.h"


class AMultiplayerPlayerState;
class UTimelineComponent;
class AMultiplayerPlayerController;
enum class TurningInPlace : uint8;
class UCombatComponent;
class AWeapon;
class UWidgetComponent;
class UInputAction;
class UInputMappingContext;
class UCameraComponent;
class USpringArmComponent;

UCLASS()
class MENUSYSTEM_API AMultiplayerCharacter : public ACharacter, public  IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	AMultiplayerCharacter();

	void PostInitializeComponents() override;
	void RotateInPlace(float DeltaTime);

	virtual void Tick(float DeltaTime) override;

	virtual void Destroyed() override;
	
	void virtual  GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	// тут мы пропишем включение SimProxiesTurn для отключение вращение корпусом, включение анимации ног для SimulatedProxy
	void virtual OnRep_ReplicatedMovement() override;
	
	void SetOverlappingWeapon(AWeapon* Weapon);
	// функция экипировки оружия запускаемое из PlayerController
	void EquipWeapon();
	// серверная функция экипировки
	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon();

	UFUNCTION(BlueprintCallable)
	AWeapon* GetWeapon();
	// задания булевой прицеливания для анимации в CombatComponent
	UFUNCTION()
	void SetIsAiming(bool bIsAiming);
	// получение булевой прицеливания для анимации в CombatComponent
	bool GetIsAiming();

	void FireButtonPressed(bool IsPressed);
	UFUNCTION(BlueprintImplementableEvent)
	void FireMontagePlayBlueprint(FName SlotName,UAnimMontage* FireAnimation);
	// функция запуска монтажа попадания на всех клиентах
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHitMontagePlay();
	
	// введем серверную функцию чтобы, воспроизводить анимацию, а потом через него сделаем NetMulticast чтобы воспроизвести у всех
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFireMontagePlay();

	// Запускается когда хп = 0 и запускает монтаж смерти
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();
	void ElimMontagePlay();
	void Elim();

	// получение точки попадания c оружейного компонента
	FVector GetHitLocation() const;
	
	FORCEINLINE float GetAO_Yaw(){ return AO_Yaw;}
	FORCEINLINE float GetAO_Pitch(){ return AO_Pitch;}
	FORCEINLINE TurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE bool IsFireButtonPressed() const { return bFireButtonPressed; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsCharacterEliminated() const { return bIsEliminated;}
	UFUNCTION(BlueprintCallable)
	UCombatComponent* GetCombatComponent();
	// получение компоненты камеры для изменения FOV
	UCameraComponent* GetCameraComponent();
	void UpdateHUDHealth();

	/*
	 * Reload
	 */
	UPROPERTY(EditDefaultsOnly, Category=Reload)
	TObjectPtr<UAnimMontage> ReloadMontage;
	UFUNCTION()
	void PlayReloadMontage();

	/*
	 * HUD
	 */
	//функция которая будет пытаться обновить счет в начале игре, потом перестанет работать
	void PollInit();


	// 5.1 получение булевой выключения управление и вращения когда GameState == Cooldown
	bool IsDisabledGameplay();
	// 16.1 функция которая создаст виджет и воспроизвете анимацию появление прицела
	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool isAiming);
protected:
	virtual void BeginPlay() override;
	// функция, которая будет скрывать персонажа и оружие когда камера будет близко к игроку
	void HideCameraIfCharacterClose();
	// функция проигрывания монтажа и секции
	void HitReactPlayMontage();
	// отключение вращение корпуса для SimulatedProxy и включение анимации ног для них 
	void SimProxiesTurn();
	
	UFUNCTION()
	void OnRep_Health();
	UFUNCTION()
	void OnRep_MaxHealth();
	// функция получения урона
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	// функция которая запускатся при окончании ElimTimer после смерти
	void ElimTimerFinisher();

	


	
	// стрела для камеры
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TObjectPtr<USpringArmComponent> CameraBoom;
	// сама камера
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;
	// виджет для отображения роли
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TObjectPtr<UWidgetComponent> OverheadWidgetComponent;
	//  боевой компонент, пока в нем будет функционал экипировки оружия
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TObjectPtr<UCombatComponent> CombatComponent;

	UPROPERTY(ReplicatedUsing=OnRep_OverlappingWeapon);
	TObjectPtr<AWeapon> OverlappingWeapon;

	UPROPERTY(EditDefaultsOnly,Category = "Animation")
	TObjectPtr<UAnimMontage> HitMontage;

	UPROPERTY(EditDefaultsOnly,Category = "Animation")
	TObjectPtr<UAnimMontage> ElimMontage;
	
	//
	// AimOffset
	//
	float AO_Yaw;
	float AO_Pitch;
	float Interp_AO_Yaw;
	FRotator StartingCharacterRotation;
	// Перечисление с положениеv куда надо повернуть ногами персонажу когда AO_Yaw > 90 и < -90
	TurningInPlace TurningInPlace;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> FireMontage;

	bool bFireButtonPressed;
	// дистанция по которой будет включаться HideCameraIfCharacterClose
	UPROPERTY(EditDefaultsOnly)
	float CameraThreshold = 200.f;

	//
	// для корректировки Rotate персонажа у SimulatedProxy
	//
	bool bRotateRootBone;
	FRotator RotationActorLastFrame = FRotator::ZeroRotator;
	FRotator RotationActorCurrentFrame = FRotator::ZeroRotator;
	float DeltaRotationBetweenFrames;
	float RotationThreshold = 0.5f;
	float DeltaTimeBetweenReplicate;

	//
	// здоровье и др показатели персонажа
	UPROPERTY(ReplicatedUsing="OnRep_Health")
	float Health = 100.f;
	UPROPERTY(ReplicatedUsing="OnRep_Health")
	float MaxHealth = 100.f;
	
	UPROPERTY()
	AMultiplayerPlayerController* MultiplayerPlayerController;

	// переменная которая будет отключать FABRIC в AnimInstance в BlendbyBool
	bool bIsEliminated = false;
	// таймер который будет отсчитывать EliminatedDelay после смерти и заново спавнить игрока
	FTimerHandle EliminatedTimer;
	UPROPERTY(EditDefaultsOnly)
	float EliminatedDelay = 3.f;
	
	/*
	 * DissolveEffect
	 */
	// кривая для Timeline там меняется переменная Disolve
	UPROPERTY(EditDefaultsOnly, Category=Elim)
	UCurveFloat* DissolveCurve;
	// материал который мы назначаем в BP чтобы превратить его в динамический материал
	UPROPERTY(EditDefaultsOnly, Category=Elim)
	UMaterialInstance* DissolveMaterialInstance;
	// динамический материал
	UPROPERTY(VisibleAnywhere, Category=Elim)
	UMaterialInstanceDynamic* DissolveDynamicMaterialInstance;
	// компонент который привязывает делегат и стартует TImeline
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimelineComponent;
	// делегат который каждый тик активируется Timerline-ом
	FOnTimelineFloat DissolveTimelineEvent;

	// функция назначения делегата и страта DIssolve
	void StartDissolve();
	// функция активируемая делегатом TimelineDissolve
	UFUNCTION()
	void UpdateDissolveMaterial(float Value);

	/*
	 *  Elim бот эффект и звук
	 */
	
	UPROPERTY(EditDefaultsOnly, Category=Elim)
	USoundBase* ElimSound;
	UPROPERTY(EditDefaultsOnly, Category=Elim)
	UParticleSystem* ElimBotParticle;
	UPROPERTY(VisibleAnywhere, Category=Elim)
	UParticleSystemComponent* ElimBotParticleComponent;
	/*
	 *
	 */
	AMultiplayerPlayerState* MultiplayerPlayerState;
private:
	
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* OldValue);
	float CalculateSpeed();
	void CalculateAO_Pitch();
	void AimOffset(float DeltaTime);
	// функция переключения пересечением с положением куда надо повернуть ногами персонажу когда AO_Yaw > 90 и < -90
	void TurnInPlace(float DeltaTime);

	

};







































