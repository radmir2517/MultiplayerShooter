// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTypes/TurningInPlace.h"
#include "MultiplayerAnimInstance.generated.h"


class AWeapon;
class AMultiplayerCharacter;

UCLASS()
class MENUSYSTEM_API UMultiplayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;


private:
	
	UPROPERTY(BlueprintReadOnly, Category= Character, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AMultiplayerCharacter> MultiplayerCharacter;

	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsInAir;

	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsAccelerating;
	
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
    bool bWeaponEquipped;

	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	bool bCrouching;

	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	bool bAiming;
	
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsLocallyControlled;

	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	bool bRotateRootBone;
	// bool чтобы в AnimInstance отключить FABRIC во время смерти
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsCharacterEliminated;
	// bool чтобы в AnimInstance отключить FABRIC во время перезарядки
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsReloading;
	
	// это для FABRIK IK, левой руки
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AWeapon> EquipWeapon;
	// это для FABRIK IK, левой руки
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	FTransform LeftHandSocketTransform;
	

	// разница в градусах, в направление движения и направления камеры для воспроизведение анимации бега вбок и назад
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	float YawOffset;
	// разница между направлениями движениями персонажа в последнем и предпоследнем кадре, чтобы наклонить персонажа в эту сторону немного
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	float Lean;
	// промежуточная переменная для YawOffset
	FRotator DeltaRotation;
	// промежуточная переменная для Lean
	FRotator LastControlRotation;
	FRotator CurrentControlRotation;

	// значения Yaw при стояние на месте  и разница между поворотом камеры и бывшим двжиение камеры
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	float AO_Yaw;
	// значение GetBaseAimRotation().Pitch;
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	float AO_Pitch;
	
	// Перечисление с положениеv куда надо повернуть ногами персонажу когда AO_Yaw > 90 и < -90
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	TurningInPlace TurningInPlace;

	//
	// это для поправления положения оружия в правой руки
	//
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	FTransform RightHandTransform;
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	FRotator RightHandRotation;


	//5.1 булевая когда игра закончилась, чтобы перестать вращать руку
	bool bDisableGameplay = false;
	
	UPROPERTY(BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess = "true"))
	bool bTransformRightHand = false;
};












































