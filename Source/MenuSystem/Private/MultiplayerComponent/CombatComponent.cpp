// Fill out your copyright notice in the Description page of Project Settings.

#include "MultiplayerComponent/CombatComponent.h"

#include "Camera/CameraComponent.h"
#include "Character/MultiplayerCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HUD/MultiplayerHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/MultiplayerPlayerController.h"
#include "Weapon/Weapon.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	if (MultiplayerCharacter)
	{
		MultiplayerCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent,Weapon);
	DOREPLIFETIME(UCombatComponent,bIsAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent,CarriedAmmo,COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent,CombatState);
	//DOREPLIFETIME(UCombatComponent,BulletSpawnTransform);
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (MultiplayerCharacter)
	{
		// получим стандартное значение FOV и назначим в начале игры как Current
		DefaultFov = MultiplayerCharacter->GetCameraComponent()->FieldOfView;
		CurrentFov = DefaultFov;
		// добавим значение в карту с типом оружием и кол-во патронов для сервера
		if (MultiplayerCharacter->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// для того чтобы каждый кадр оружие было направлено точно в цель мы вернем TraceUnderCrosshairs в Tick
	if (MultiplayerCharacter && MultiplayerCharacter->IsLocallyControlled())
	{
		TraceUnderCrosshairs(HitResult);
		HitLocation = HitResult.ImpactPoint;
		
		ChangeFOVForAiming(DeltaTime);
		SetHUDCrosshairs(DeltaTime);
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	// проверим что все указатели рабочие и отправим части прицела в HUD
	if (MultiplayerCharacter && MultiplayerCharacter->Controller)
	{
		MultiplayerPlayerController = MultiplayerPlayerController == nullptr ?  Cast<AMultiplayerPlayerController>(MultiplayerCharacter->Controller) : MultiplayerPlayerController;
		if (MultiplayerPlayerController)
		{
			MultiplayerHUD = MultiplayerHUD == nullptr ?  Cast<AMultiplayerHUD>(MultiplayerPlayerController->GetHUD()) : MultiplayerHUD ;
			if (MultiplayerHUD)
			{	
				// если есть оружие, то будем оттуда брать прицел для каждого оружия если нет то пустые указатели
				if (Weapon)
				{
					HUDPackage = Weapon->GetHudPackage();
				}
				else
				{
					HUDPackage.CrosshairsCenter = nullptr;
					HUDPackage.CrosshairsTop = nullptr;
					HUDPackage.CrosshairsBottom = nullptr;
					HUDPackage.CrosshairsLeft = nullptr;
					HUDPackage.CrosshairsRight = nullptr;
				}

				// проверим при трассировке задеваем ли мы актер и если задеваем, то игрок ли это
				if (HitResult.GetActor() && HitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
				{	// если игрок то прицел станет красным
					HUDPackage.CrosshairsColor = FLinearColor::Red;
				}
				else
				{	// если нет то белым
					HUDPackage.CrosshairsColor = FLinearColor::White;
				}
				
				// Отдаление прицела при ходьбе и прыжка
				//
				FVector2D RangeMinMaxToSpeed(0.f,600.f);
				FVector2D RangeMinMaxToSpreadOfAim(0.f,1.f);
				FVector Velocity = MultiplayerCharacter->GetCharacterMovement()->Velocity;
				Velocity.Z = 0.f;
				float Speed = Velocity.Size();
				
				// сделаем соответсвие скорости к коээфициенту розлета прицела от 0 до 1 
				CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(RangeMinMaxToSpeed, RangeMinMaxToSpreadOfAim, Speed);
				// если персонаж в полете то увеличим прицел больше обычного
				if (MultiplayerCharacter->GetCharacterMovement()->IsFalling())
				{
					
					FlySpreadFactor = FMath::FInterpTo(FlySpreadFactor, 2.25f,DeltaTime, 6.f);
				}
				else
				{
					FlySpreadFactor = FMath::FInterpTo(FlySpreadFactor, 0, DeltaTime, 30.f);
				}
				if (bIsAiming)
				{
					CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor,0.58f,DeltaTime, 30.f);
				}
				else
				{
					CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor,0,DeltaTime, 30.f);
				}

				// будем уменьшать до 0 коэфф увеличение прицела при стрельбе, а когда стрельба идет он станет 0.75 в FireButtonPressed()
				CrosshairShootingFactor = FMath::FInterpTo(CrosshairAimFactor,0,DeltaTime, 30.f);
				
				HUDPackage.SpreadFactor =
					0.5f +
					CrosshairVelocityFactor +
					FlySpreadFactor -
						CrosshairAimFactor +
							CrosshairShootingFactor;
				
				// отправим в HUD указатели картинок прицела
				MultiplayerHUD->SetHUDPackage(HUDPackage);
			}
		}
	}
}

void UCombatComponent::ChangeFOVForAiming(float DeltaTime)
{
	if (!MultiplayerCharacter->GetWeapon()) return;
	
	// при прицеливании 
	if (bIsAiming)
	{	// мы получим FOV оружия и поменяем стандартное значение на оружие
		float WeaponAIMFOV = GetWeapon()->GetAimFOV();
		CurrentFov = FMath::FInterpTo(CurrentFov,WeaponAIMFOV , DeltaTime, GetWeapon()->GetInterpFOVSpeed());
	}
	else
	{	// если не целимся то возвращаемся к стандартному
		CurrentFov = FMath::FInterpTo(CurrentFov, DefaultFov, DeltaTime, GetWeapon()->GetInterpFOVSpeed());
	}
	MultiplayerCharacter->GetCameraComponent()->FieldOfView = CurrentFov;
}


void UCombatComponent::EquipWeapon(AWeapon* InWeapon)
{	// проверим что не нулевые
	if (!IsValid(MultiplayerCharacter) || !IsValid(InWeapon)) return;
	// если в руках уже есть оружие, то выкинем его и поберем новое
	if (Weapon)
	{
		Weapon->Dropped();
	}
	
	// назначим переменную оружия
	Weapon = InWeapon;
	// назначим владельца, поменяем статус чтобы он больше не показывал надпись, отключим физику оружия и уберем показывания оружия
	Weapon->SetOwner(MultiplayerCharacter);
	Weapon->SetHUDAmmo_Public();
	Weapon->SetWeaponState(EWeaponState::EWC_Equipped);
	Weapon->ShowPickUpWidget(false);

	// получим значение с карты с CarriedAmmoMap
	if (CarriedAmmoMap.Contains(Weapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[Weapon->GetWeaponType()];
	}
	// назначим значение CarriedAmmo для сервера
	MultiplayerPlayerController = MultiplayerPlayerController == nullptr ?  Cast<AMultiplayerPlayerController>(MultiplayerCharacter->Controller) : MultiplayerPlayerController;
	if (MultiplayerPlayerController)
	{
		MultiplayerPlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	}

	if (Weapon->GetPickUpSound())
	{
		UGameplayStatics::SpawnSoundAttached(Weapon->GetPickUpSound(),MultiplayerCharacter->GetRootComponent());
	}

	if (Weapon->IsEmpty())
	{
		Reload();
	}
	
	// получим сокет из персонажа и проверим что он существует
	const USkeletalMeshSocket* HandSocket = MultiplayerCharacter->GetMesh()->GetSocketByName( FName("RightHandSocket"));
	if (HandSocket)
	{	// прикрепим к сокету оружие наше
		HandSocket->AttachActor(Weapon,MultiplayerCharacter->GetMesh());
	}
	// отключение у сервера(клиенты в OnRep_Weapon) ориентации по направлению, после подбора и включение поворота персонажа по повороту мыши,
	// чтобы теперь совпадали поворот мыши и персонажа
	MultiplayerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	MultiplayerCharacter->bUseControllerRotationYaw = true;
}


void UCombatComponent::OnRep_Weapon()
{
	if (MultiplayerCharacter && MultiplayerCharacter->GetCharacterMovement())
	{
		// отключение у клиентов ориентации по направлению, после подбора и включение поворота персонажа по повороту мыши,
		// чтобы теперь совпадали поворот мыши и персонажа
		MultiplayerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		MultiplayerCharacter->bUseControllerRotationYaw = true;

		// меняем статус на всякий тоже и тут чтобы у клиентов не включилась физика у оружии при подборе
		Weapon->SetWeaponState(EWeaponState::EWC_Equipped);
		const USkeletalMeshSocket* HandSocket = MultiplayerCharacter->GetMesh()->GetSocketByName( FName("RightHandSocket"));
		if (HandSocket)
		{	// прикрепим к сокету оружие наше
			HandSocket->AttachActor(Weapon,MultiplayerCharacter->GetMesh());
		}
		
		if (Weapon->GetPickUpSound())
		{
			UGameplayStatics::SpawnSoundAttached(Weapon->GetPickUpSound(),MultiplayerCharacter->GetRootComponent());
		}
		// если после подбора пустая обойма перезарядим ее
		if (Weapon->IsEmpty())
		{
			Reload();
		}
	}
}

void UCombatComponent::ServerSetIsAiming_Implementation(bool InbAim)
{
	bIsAiming = InbAim;
	// если целимся, то назначаем скорость другую
	if (MultiplayerCharacter)
	{
		MultiplayerCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::SetIsAiming(bool InbAim)
{
	bIsAiming = InbAim;
	// если целимся, то назначаем скорость другую
	if (MultiplayerCharacter)
	{
		MultiplayerCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::FireButtonPressed(bool IsPressed)
{
	bIsFirePressed = IsPressed;
	// трассировка с центра экрана по направлению вперед UGameplayStatics::DeprojectScreenToWorld и нарисуем сферу 
	// если кнопка прожата то запустим таймер выстрела, автоматической стрельбы или ручной
	
	if (MultiplayerCharacter && MultiplayerCharacter->IsLocallyControlled() && bIsFirePressed )
	{ // если кнопка прожата, то запустим таймер выстрела, автоматической стрельбы или ручной
		StartAutomaticFire();
	}
}

void UCombatComponent::StartAutomaticFire()
{	// проверим что у нас есть оружие и мы можем стрелять и то что у нас есть патроны в магазине
	if ( GetWeapon() && CanFire())
	{	// пока таймер не отсчитал свое время сделаем первый выстрел
		WeaponStartFire();
		// запустим таймер, время возьмем с оружия, inLoop будет зависеть стоит ли галка автоматической стрельбы
		MultiplayerCharacter->GetWorldTimerManager().SetTimer(FireTimer,this,&UCombatComponent::FinishAutomaticFire,GetWeapon()->GetFireDelay(),GetWeapon()->ShouldAutomaticFire());
		// пометим как false чтобы между промежутками нельзя было стрелять вручную
		bCanFire=false;
		// StartAutomaticFire запускается лишь один раз при нажатии поэтому это будет первый выстрел
		bFirstFire=true;
	}
}

void UCombatComponent::FinishAutomaticFire()
{
	// пометим как true чтобы выстрел по таймеру был разрешен
	bCanFire = true;
	// если это был первый выстрел, то первую итерацию по таймеру пропустим т.к мы запустили на один раз вручную
	if (bFirstFire)
	{
		bFirstFire = false;
		return;
	}
	// если к этому времени мы уже отпустили кнопку или нет патронов, то выстрела не будет
	if (!CanFire() || !bIsFirePressed )
	{	// если кнопка от
		MultiplayerCharacter->GetWorldTimerManager().ClearTimer(FireTimer);
		return;
	}
	// запустим выстрел, т.е трассировку, увеличение разлет прицела, спавн пули и эффекта
	WeaponStartFire();
	// если обойма пуста запустим перезарядку
	if (Weapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::WeaponStartFire()
{
	// трассировка с середины экрана и направлена далее пока не столкнется
	TraceUnderCrosshairs(HitResult);
	HitLocation = HitResult.ImpactPoint;
	// установим коэфф увеличение прицела при стрельбе
	CrosshairShootingFactor = 0.75f;
		
	if (HitResult.bBlockingHit)
	{
		//HitTarget = HitResult.ImpactPoint;
		Fire(bIsFirePressed, HitLocation);
	}
}

bool UCombatComponent::CanFire()
{
	if (GetWeapon() && !GetWeapon()->IsEmpty() && bCanFire && CombatState == ECombatState::ECT_Unoccupied)
	{
		return true;
	}
	return false;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	// назначим значение CarriedAmmo для сервера
	MultiplayerPlayerController = MultiplayerPlayerController == nullptr ?  Cast<AMultiplayerPlayerController>(MultiplayerCharacter->Controller) : MultiplayerPlayerController;
	if (MultiplayerPlayerController)
	{
		MultiplayerPlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	// добавим значение в карту
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	// 7.1 добавим значение в карту
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRLAmmo);
	// 10.1 добавим запасные патроны для пистолета 
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPAmmo);
}

void UCombatComponent::Fire(bool IsFireButtonPressed, const FVector_NetQuantize& TargetPoint)
{
	if (GetWeapon() && IsFireButtonPressed)
	{
		// запускаем серверную функцию спавна пули
		ServerFire(TargetPoint);
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TargetPoint)
{
	// обновляем вращение и локацию для спавна пули
	//CombatComponent->BulletSpawnTransform = BulletTransform;
	// далее проверяем что все ок и запускаем спавн пули
	if (GetWeapon() && MultiplayerCharacter)
	{	// спавн пули
		GetWeapon()->OpenFire(TargetPoint);

		// запускаем NetMulticast функцию для воспроизведения на всех клиентах
		MultiplayerCharacter->MulticastFireMontagePlay();
	}

}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& InHitResult)
{
	// вектор в котором мы будем хранить размер экрана
	FVector2D ViewportSize;
	// здесь будет центр экрана
	FVector2D CrosshairOffset;
	
	if (GEngine && GEngine->GameViewport)
	{	// получим размеры экрана и центр его
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		CrosshairOffset = FVector2D(ViewportSize.X/2,ViewportSize.Y/2);
	}
	// вектора в котором мы центр экрана превратим в координаты в 3 мире и получим направление вперед
	FVector WorldPosition;
	FVector WorldDirection;
	FVector EndPosition;
	
	if (!ViewportSize.IsZero())
	{	// получим координаты экрана в 3д мире и направление его в вперед
		bool bIsDeeprojectctSuccess = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this,0),
			CrosshairOffset,
			WorldPosition,
			WorldDirection
			);
		EndPosition = WorldDirection * TRACE_LENGHT;

		// передвинем начала трассировки чтобы не задевала людей кто сзади камеры передвинем начало трассивроки вперед игрока
		if (MultiplayerCharacter)
		{
			float DistanceToPlayer = (WorldPosition - MultiplayerCharacter->GetActorLocation()).Size();
			WorldPosition = WorldPosition + WorldDirection * (DistanceToPlayer + 90.f) ;
		}
		
		if (bIsDeeprojectctSuccess)
		{	// сделаем трассировку с центра экрана вперед на большую длину в 50к и получим HitResult и версем по ссылке
			FCollisionQueryParams CollisionParams = FCollisionQueryParams::DefaultQueryParam;
			CollisionParams.AddIgnoredActor(MultiplayerCharacter);
			
			GetWorld()->LineTraceSingleByChannel(InHitResult,
				WorldPosition,
				WorldPosition + EndPosition,
				ECC_Visibility, CollisionParams);
			
			if (!InHitResult.bBlockingHit)
			{ // если нет пересечения, то конец просто будет в расстояние 50к
				InHitResult.ImpactPoint = WorldPosition + EndPosition;
			}
			else 
			{	// если столкновение есть, то нарисуем там сферу
			}
			
		}
	}
}

void UCombatComponent::Reload()
{
	if ( GetWeapon() && CarriedAmmo > 0 && CombatState != ECombatState::ECT_Reloading  )
	{
		Server_Reload();
	}
}

void UCombatComponent::Server_Reload_Implementation()
{
	if (!IsValid(MultiplayerCharacter)) return;
	// запустим функцию, которая запустит анимацию воспроизведения
	CombatState = ECombatState::ECT_Reloading;
	// запустим анимацию у сервера
	HandleReload();
}

void UCombatComponent::HandleReload()
{
	MultiplayerCharacter->PlayReloadMontage();
}

void UCombatComponent::OnRep_CombatState()
{
	if (!IsValid(MultiplayerCharacter)) return;
	// если у нас стал Reloading то запустим анимацию у клиентов
	switch (CombatState)
	{
		case ECombatState::ECT_Reloading:
		HandleReload();
		break;
		
		case ECombatState::ECT_Unoccupied:
		if (bIsFirePressed)
		{
			StartAutomaticFire();
		}
		break;
	}
}

void UCombatComponent::FinishReloading()
{
	if (!IsValid(MultiplayerCharacter)) return;
	// через сервер переключим статус 
	if (MultiplayerCharacter->HasAuthority())
	{
		// механика пополнения магазина в числах, CArriedAmmo и просто ammo
		UpdateAmmoValue();
		CombatState = ECombatState::ECT_Unoccupied;
	}
	 // а если нажата кнопка то начнем стрельбу
	if (bIsFirePressed && MultiplayerPlayerController && !MultiplayerPlayerController->IsDisableGameplay())
	{
		StartAutomaticFire();
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (GetWeapon())
	{
		int32 RoomInMag = GetWeapon()->GetMaxMagAmmo() - GetWeapon()->GetCurrentAmmo();
		RoomInMag = FMath::Clamp(RoomInMag, 0, RoomInMag);
		return RoomInMag;
	}
	return 0;
}

void UCombatComponent::UpdateAmmoValue()
{
	if (!GetWeapon() && AmountToReload() == 0 && MultiplayerPlayerController == nullptr) return;
	// получим меньшее значение либо из сколько нам надо в магазине либо сколько осталось в запасе
	int32 Least = FMath::Min(AmountToReload(), CarriedAmmo);
	int32 AmmoInMagazineNow = GetWeapon()->GetCurrentAmmo();
	// вычтем запасные патроны
	if (CarriedAmmoMap.Contains(GetWeapon()->GetWeaponType()))
	{
		CarriedAmmoMap[GetWeapon()->GetWeaponType()] -= Least;
		CarriedAmmo = CarriedAmmoMap[GetWeapon()->GetWeaponType()];
	}
	
	// обновим значение Carried у сервера, а клиента обновиться уже в OnRep_CarriedAmmo
	MultiplayerPlayerController = MultiplayerPlayerController == nullptr ?  Cast<AMultiplayerPlayerController>(MultiplayerCharacter->Controller) : MultiplayerPlayerController;
	if (MultiplayerPlayerController)
	{
		MultiplayerPlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	}
	// и прибавим патроны в текущий магазин.
	GetWeapon()->SetCurrentAmmo(AmmoInMagazineNow + Least);
	// обновим значение WeaponAmmo у сервера, а клиента обновиться уже в OnRep_WeaponAmmo
	if (MultiplayerPlayerController)
	{
		MultiplayerPlayerController->SetHUDWeaponAmmo(AmmoInMagazineNow + Least);
	}
}





















