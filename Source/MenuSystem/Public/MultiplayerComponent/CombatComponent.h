// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTypes/CombatState.h"
#include "GameplayTypes/WeaponTypes.h"
#include "HUD/MultiplayerHUD.h"
#include "CombatComponent.generated.h"


class AProjectile;
class AMultiplayerHUD;
class AMultiplayerPlayerController;
class AWeapon;
class AMultiplayerCharacter;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MENUSYSTEM_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// функция экипировки оружия, активируется из Character
	void EquipWeapon(AWeapon* InWeapon);

	void EquipPrimaryWeapon(AWeapon* InWeapon);
	void EquipSecondaryWeapon(AWeapon* InWeapon);
	// сеттер и геттер для персонажа владеющей компонентом
	FORCEINLINE void SetMultiplayerCharacter(AMultiplayerCharacter* InMultiplayerCharacter)
	{
		MultiplayerCharacter = InMultiplayerCharacter;
	}

	FORCEINLINE AMultiplayerCharacter* GetMultiplayerCharacter() { return MultiplayerCharacter; }
	FORCEINLINE AWeapon* GetWeapon() { return PrimaryEquipWeapon; }
	FORCEINLINE AWeapon* GetSecondaryWeapon() { return SecondaryEquipWeapon; }
	FORCEINLINE bool GetIsAiming() { return bIsAiming; }
	FORCEINLINE ECombatState GetCombatState() { return CombatState; }
	FORCEINLINE int32 GetGrenadesAmount() { return GrenadesAmount; }
	void SetIsAiming(bool bInAim);
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
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing=OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECT_Unoccupied;

	void Reload();
	UFUNCTION(Server, Reliable)
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
	//18.1 Функция перезарядки вызываемая в ABPAnimInstance
	UFUNCTION(BlueprintCallable)
	void ShotgunReloading();
	//18.2 функция пополнения мгазаина по 1 патрону в дробовике и показания это в hud
	void UpdateShotgunAmmoValue();

	//19.1 запуск функции броска гранаты при нажатии кнопки броска
	void ThrowGrenade();
	// 19.2 Запуск изменения состояние на сервере
	UFUNCTION(Server,Reliable)
	void Server_ThrowGrenade();
	// 19.3 тут будет запуск анимации ан клиентах и на сервере
	void HandleThrowGrenade();
	// 19.4 окончания анимации броска гранаты для возврата состояния
	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();
	// 21.1 Создадим функцию изменения видимости граната в начале броска и в конце
	void SetVisibilityToGrenade(bool bVisibilityGrenade);
	// 21.4 функция бросания гранаты запускаемая с анимации
	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();
	// 23.1 Сделаем серверную функцию чтобы клиент мог передать серверу место попадания
	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(FVector_NetQuantize InHitLocation);
	
	//25.1 функция срабатывает когда поднимаем патроны 
	UFUNCTION()
	void PickUpAmmo(EWeaponType WeaponType, int32 AmmoAmount);
	// 20.1 функции созданные для EquipWeapon для сокращения написания кода в EquipWeapon
	bool GetAndUpdateHudCarriedAmmo();

	UFUNCTION(Server, Reliable)
	void SwapWeapon();
protected:
	virtual void BeginPlay() override;
	// функция репликация для оружия
	UFUNCTION()
	void OnRep_PrimaryEquipWeapon();
	// функция репликация для оружия
	UFUNCTION()
	void OnRep_SecondaryEquipWeapon();

	
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
	UPROPERTY()
	TObjectPtr<AMultiplayerCharacter> MultiplayerCharacter;
	// оружие которое будет назначаться и потом экипироваться

	UPROPERTY(ReplicatedUsing=OnRep_PrimaryEquipWeapon, BlueprintReadOnly)
	TObjectPtr<AWeapon> PrimaryEquipWeapon;
	
	UPROPERTY(ReplicatedUsing=OnRep_SecondaryEquipWeapon, BlueprintReadOnly)
	TObjectPtr<AWeapon> SecondaryEquipWeapon;
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

	// функция инцилизации CarriedAmmoMap, добавим туда тип оружия нашего 
	void InitializeCarriedAmmo();

	// карта в котором в зависимости от типа оружие будет менятся и макс кол-во патронов
	TMap<EWeaponType, int32> CarriedAmmoMap;

	// пока временное значение для CarriedAmmo для оружия
	UPROPERTY(EditDefaultsOnly, Category="CarriedAmmo")
	int32 StartingARAmmo = 30;
	// 7.1 пока временное значение для CarriedAmmo для оружия
	UPROPERTY(EditDefaultsOnly, Category="CarriedAmmo")
	int32 StartingRLAmmo = 4;
	// 10.1 Добавим патроны для пистолета
	UPROPERTY(EditDefaultsOnly, Category="CarriedAmmo")
	int32 StartingPistolAmmo = 15;
	// 12.1 Добавим патроны для пистолета
	UPROPERTY(EditDefaultsOnly, Category="CarriedAmmo")
	int32 StartingSMGAmmo = 30;
	// 13.1 Добавим патроны для дробовика
	UPROPERTY(EditDefaultsOnly, Category="CarriedAmmo")
	int32 StartingShotgunAmmo = 12;
	// 15.1 Добавим патроны для cнайперской винтовки
	UPROPERTY(EditDefaultsOnly, Category="CarriedAmmo")
	int32 StartingSniperRifleAmmo = 5;
	// 17.1 Добавим патроны для Запускателя гранат
	UPROPERTY(EditDefaultsOnly, Category="CarriedAmmo")
	int32 StartingGrenadeLauncherAmmo = 5;
	
	/*
	 * Гранаты
	 */
	// 21.0 Добавим переменную класса гранаты для спавна
	UPROPERTY(EditDefaultsOnly, Category="HandGrenade")
	TSubclassOf<AProjectile> GrenadeClass;
	// 24.5 кол-во гранат, оно будет менять и реплицироваться
	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_GrenadesAmount, Category="HandGrenade")
	int32 GrenadesAmount = 4;
	// 24.6 кол-во макс гранат для проверки
	UPROPERTY(EditDefaultsOnly, Category="HandGrenade")
	int32 GrenadesMaxAmount = 4;

	void UpdateHUDGrenadesAmount();
	// 24.7 Функция репликация для гранат
	UFUNCTION()
	void OnRep_GrenadesAmount();

	
	
private:
	bool bIsStartingReloading = false;

	// 20.1 функции созданные для EquipWeapon для сокращения написания кода в EquipWeapon
	void DropWeapon();
	void SpawnPickUpSound(AWeapon* InEquippedWeapon);
	void ReloadAmmoIfEmpty();
	void AttackWeaponAtSocket(AWeapon* InWeapon,FName SocketName);
};








































