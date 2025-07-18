// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTypes/CombatState.h"
#include "GameplayTypes/WeaponTypes.h"
#include "HUD/MultiplayerHUD.h"
#include "CombatComponent.generated.h"


class AMultiplayerHUD;
class AMultiplayerPlayerController;
class AWeapon;
class AMultiplayerCharacter;

#define  TRACE_LENGHT 60000.f

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MENUSYSTEM_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	// функция экипировки оружия, активируется из Character
	void EquipWeapon(AWeapon* InWeapon);
	// сеттер и геттер для персонажа владеющей компонентом
	FORCEINLINE void SetMultiplayerCharacter(AMultiplayerCharacter* InMultiplayerCharacter) { MultiplayerCharacter = InMultiplayerCharacter; }
	FORCEINLINE AMultiplayerCharacter* GetMultiplayerCharacter() { return MultiplayerCharacter; }
	FORCEINLINE AWeapon* GetWeapon() {return Weapon;}
	FORCEINLINE bool GetIsAiming() {return bIsAiming;}
	void SetIsAiming(bool InbAim);
	UFUNCTION(Server, Reliable)
	void ServerSetIsAiming(bool InbAim);
	// функция вызыва
	void FireButtonPressed(bool IsPressed);

	void Fire(bool IsFireButtonPressed, const FVector_NetQuantize& TargetPoint);
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TargetPoint);
		
	//
	// Стрельба
	// результат трассировки с центра экрана вперед до пересечения чего либо

	// трассировка с центра экрана по направлению вперед UGameplayStatics::DeprojectScreenToWorld
	void TraceUnderCrosshairs(FHitResult& InHitResult);
	
	FHitResult HitResult;
	FVector HitLocation;
	// переменная полученная с оружия, для репликации на другие клиенты
	//FTransform BulletSpawnTransform;

	/*
	 *Reload
	 */
	// состояние персонажа. Перезаряжающий, незадействованный
	UPROPERTY(BlueprintReadWrite,ReplicatedUsing=OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECT_Unoccupied;
	
	void Reload();
	UFUNCTION(Server,Reliable)
	void Server_Reload();

	void HandleReload();
	
	UFUNCTION()
	void OnRep_CombatState();
	// запустим из BP_AnimInstance во время окончания монтажа
	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	// функция посдчета сколько надо для пополнения магазина 
	int32 AmountToReload();
	// пополнение магазина и обновление переменных у клиента и в Overlay
	void UpdateAmmoValue();
	
protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnRep_Weapon();
	//функция которая будет получать с оружия картинки прицела и отправлять в HUD каждый кадр
	void SetHUDCrosshairs(float DeltaTime);
	// изменения FOV при прицеливания
	void ChangeFOVForAiming(float DeltaTime);

	//
	//Автоматический огонь
	//
	// функция запуска таймера выстрела для автоматического огня
	void StartAutomaticFire();
	//функция запуская таймером выстрела где проверяются некоторые условия и запускается WeaponStartFire()
	void FinishAutomaticFire();
	// функция запускания выстрела Fire(), т.е трассировку, увеличение разлет прицела, спавн пули и эффекта
	void WeaponStartFire();
	
	// таймер выстрела
	FTimerHandle FireTimer;
	// булевая для запрета ручных выстрелов между самими выстрелами
	bool bCanFire = true;
	// булева которая при первом выстреле пропускает первую итерация таймера для избежаения двойног выстрела
	bool bFirstFire = true;
	
	// персонаж владеющим им
	TObjectPtr<AMultiplayerCharacter> MultiplayerCharacter;
	// оружие которое будет назначаться и потом экипироваться

	UPROPERTY(ReplicatedUsing=OnRep_Weapon, BlueprintReadOnly)
	TObjectPtr<AWeapon> Weapon;

	UPROPERTY(Replicated)
	bool bIsAiming;
	
	UPROPERTY(EditDefaultsOnly)
	float BaseWalkSpeed = 500.f;
	UPROPERTY(EditDefaultsOnly)
	float AimWalkSpeed = 300.f;

	bool bIsFirePressed = false;
	
	// указатели на контроллер и HUD
	AMultiplayerPlayerController* MultiplayerPlayerController;
	AMultiplayerHUD* MultiplayerHUD;
	// переменные в которых будет определятся отдаление прицела при движение и полета
	float CrosshairVelocityFactor;
	float FlySpreadFactor;
	// коэфф вычитания ширины прицелы при прицеливания
	float CrosshairAimFactor;
	float CrosshairShootingFactor;
	
	UPROPERTY()
	FHUDPackage HUDPackage;
	//
	//Изменение FOV для прицеливания 

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float DefaultFov;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CurrentFov;
	
	//
	// CanFire
	// дополним CanFire ограничением магазина
	bool CanFire();

	/*
	 * Carried ammo
	 */
	// боеприпасы для текущего оружия
	UPROPERTY(ReplicatedUsing="OnRep_CarriedAmmo")
	int32 CarriedAmmo;
	
	UFUNCTION()
	void OnRep_CarriedAmmo();
	
	// карта в котором в зависимости от типа оружие будет менятся и макс кол-во патронов
	TMap<EWeaponType, int32> CarriedAmmoMap;
	
	// пока временное значение для CarriedAmmo для оружия
	UPROPERTY(EditDefaultsOnly, Category="CarriedAmmo")
	int32 StartingARAmmo = 30;
	// 7.1 пока временное значение для CarriedAmmo для оружия
	UPROPERTY(EditDefaultsOnly, Category="CarriedAmmo")
	int32 StartingRLAmmo = 4;
	// 10.1 Добавим патроны для пистолета
	int32 StartingPAmmo = 15;
	// 12.1 Добавим патроны для пистолета
	int32 StartingSMGAmmo = 30;
	// функция инцилизации CarriedAmmoMap, добавим туда тип оружия нашего 
	void InitializeCarriedAmmo();
	
};




















































