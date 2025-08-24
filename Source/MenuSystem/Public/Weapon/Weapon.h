// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTypes/WeaponTypes.h"
#include "Weapon.generated.h"

class AMultiplayerPlayerController;
class AMultiplayerCharacter;
struct FHUDPackage;
class ACasing;
class UNiagaraSystem;
class UNiagaraComponent;
class UWidgetComponent;
class USphereComponent;
// перечисления для состояния оружия
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWC_Initial UMETA(DisplayName = "Initial"),
	EWC_Equipped UMETA(DisplayName = "Equipped"),
	EWC_Dropped UMETA(DisplayName = "Dropped"),

	EWC_MAX UMETA(DisplayName = "DefaultMAX")
};
UCLASS()
class MENUSYSTEM_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();

	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_Owner() override;
	
	void ShowPickUpWidget(bool bVisibilityWidget);

	void SetWeaponState(EWeaponState InWeaponState);

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	virtual void OpenFire(const FVector_NetQuantize& TargetPoint);
	// геттеры для FOV
	FORCEINLINE float GetAimFOV() const { return AimFOV; }
	FORCEINLINE float GetInterpFOVSpeed() const { return InterpFOVSpeed; }
	FORCEINLINE float GetFireDelay() const { return FireDelay;}
	FORCEINLINE bool ShouldAutomaticFire() const { return bIsAutomaticFire;}
	FORCEINLINE int32 GetCurrentAmmo() const {return WeaponAmmo;}
	FORCEINLINE int32 GetMaxMagAmmo() const {return MaxWeaponAmmo;}
	FORCEINLINE EWeaponType GetWeaponType() const {return WeaponType;}
	FORCEINLINE USoundBase* GetPickUpSound() const {return PickUpSound;}
	FORCEINLINE bool IsStandartWeapon() const {return  bIsStandardWeapon;}
	void SetCurrentAmmo(const int32 Ammo) {WeaponAmmo = Ammo;}
	//31. задания булевой что это стандартное оружие
	void SetbStandardWeapon(bool bStandardWeapon);
	
	// функция которая возвращает части прицела в структуре
	FHUDPackage GetHudPackage() const;
	
	//функция которая сработает при смерти игрока и выкенет оружие на пол
	void Dropped();

	// побочная функция где мы получаем указатели на персонаж и контроллер и обновления знач на экране
	UFUNCTION()
	void SetHUDAmmo_Public();

	// функция которая будет проверять равен ли кол-во патронов 0
	bool IsEmpty();
	bool IsFullAmmo();

	UFUNCTION(BLueprintCallable, NetMulticast, Unreliable)
	void PlayFireEffect();
	UFUNCTION(BLueprintCallable, NetMulticast, Unreliable)
	void StopFireEffect();
	
protected:
	virtual void BeginPlay() override;
	// функции для появление виджета с текстом о подсказке поднятия предмета
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	UFUNCTION()
	void OnRep_WeaponState();
	//19.1 включение/выключение влияние CustomDepth постпроцесса на оружие
	void SetRenderCustomDepth(bool bSetEnabled);
	
	
	UPROPERTY(VisibleAnywhere, Category="Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;
	UPROPERTY(VisibleAnywhere, Category="Weapon")
	TObjectPtr<USphereComponent> AreaSphere;
	UPROPERTY(VisibleAnywhere, Category="Weapon")
	TObjectPtr<UWidgetComponent> PickUpWidgetComponent;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category="Weapon")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;
	// 7.1 Если нет эффекта ниагары, а есть эффект каскады.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<UParticleSystem> SecondVariantFireEffect;

	// 12.1 звук выстрела
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly , meta=(AllowPrivateAccess, Category="Weapon"))
	TObjectPtr<USoundBase> OpenFireSound;
	
	TObjectPtr<UNiagaraSystem> NiagaraFireEffect;

	
	// класс для гильзы
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	TSubclassOf<ACasing> CasingClass;

	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	FName SocketNameOnWeapon = "FireSocket";
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	FName SocketNameForCasing = "AmmoEject";
	
	// состояние оружия по умолчанию у него будет EWC_Initial
	UPROPERTY(ReplicatedUsing=OnRep_WeaponState,VisibleAnywhere, Category="Weapon Properties")
	EWeaponState WeaponState;

	// компонент который будет создан во время выполнения эффекта
	TObjectPtr<UNiagaraComponent> FireEffect;
	
	//
	// Crosshair прицел
	//
	UPROPERTY(EditDefaultsOnly, Category=Crosshair)
	TObjectPtr<UTexture2D> CrosshairsCenter;
	UPROPERTY(EditDefaultsOnly, Category=Crosshair)
	TObjectPtr<UTexture2D> CrosshairsRight;
	UPROPERTY(EditDefaultsOnly, Category=Crosshair)
	TObjectPtr<UTexture2D> CrosshairsBottom;
	UPROPERTY(EditDefaultsOnly, Category=Crosshair)
	TObjectPtr<UTexture2D> CrosshairsTop;
	UPROPERTY(EditDefaultsOnly, Category=Crosshair)
	TObjectPtr<UTexture2D> CrosshairsLeft;
	UPROPERTY(EditDefaultsOnly, Category=Crosshair)
	float MaxSpreadMagnitude = 16.f;

	//
	// AutomaticFire
	//
	// задержка между выстрелами, для таймера
	UPROPERTY(EditDefaultsOnly, Category="Weapon Properties")
	float FireDelay = 0.15f;
	UPROPERTY(EditDefaultsOnly, Category="Weapon Properties")
	bool bIsAutomaticFire = true;
	
	// параметры FOV при прицеливания
	UPROPERTY(EditDefaultsOnly, Category="Weapon Properties")
	float AimFOV = 30.f;
	UPROPERTY(EditDefaultsOnly, Category="Weapon Properties")
	float InterpFOVSpeed = 10.f;

	/*
	 * WeaponAmmo
	 */
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_WeaponAmmo, Category="Weapon Properties")
	int32 WeaponAmmo = 30;
	UPROPERTY(EditDefaultsOnly, Category="Weapon Properties")
	int32 MaxWeaponAmmo = 30;
	UPROPERTY()
	AMultiplayerCharacter* MultiplayerCharacter;
	UPROPERTY()
	AMultiplayerPlayerController* MultiplayerPlayerController;
	
	// функция репликации боеприпаса и отправления значения на экран у клиента
	UFUNCTION()
	void OnRep_WeaponAmmo();
	// отнимаем боеприпас и и отправления значения на экран у сервера
	UFUNCTION()
	void SpendAmmo();
	// побочная функция где мы получаем указатели на персонаж и контроллер и обновления знач на экране
	UFUNCTION()
	void SetHUDAmmo();
	
	/*
	 *	CarriedAmmo
	 */
	UPROPERTY(EditDefaultsOnly,Category="Weapon Properties")
	EWeaponType WeaponType = EWeaponType::EWT_AssaultRifle;

	/*
	 *	PickUp
	 */
	// звук подбора оружия
	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
	TObjectPtr<USoundBase> PickUpSound;

	//31.6 bool для обозначения выдается ли оружие при спавне игрока или нет
	bool bIsStandardWeapon = false;
	
};






























