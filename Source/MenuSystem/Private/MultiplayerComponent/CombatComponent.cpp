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
#include "Weapon/Projectile.h"
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
	DOREPLIFETIME_CONDITION(UCombatComponent,GrenadesAmount, COND_OwnerOnly);
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


void UCombatComponent::DropWeapon()
{
	if (Weapon)
	{
		Weapon->Dropped();
	}
}

void UCombatComponent::GetAndUpdateHudCarriedAmmo()
{
	if (!IsValid(Weapon) || !MultiplayerCharacter) return;
	
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
}

void UCombatComponent::SpawnPickUpSound()
{
	if (Weapon && Weapon->GetPickUpSound() && MultiplayerCharacter)
	{
		UGameplayStatics::SpawnSoundAttached(Weapon->GetPickUpSound(),MultiplayerCharacter->GetRootComponent());
	}
}

void UCombatComponent::ReloadAmmoIfEmpty()
{
	if (Weapon && Weapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::AttackWeaponAtSocket(FName SocketName)
{
	if (!MultiplayerCharacter || !Weapon) return;
	const USkeletalMeshSocket* HandSocket = MultiplayerCharacter->GetMesh()->GetSocketByName( SocketName);
	if (HandSocket)
	{	// прикрепим к сокету оружие наше
		HandSocket->AttachActor(Weapon,MultiplayerCharacter->GetMesh());
	}
}

void UCombatComponent::EquipWeapon(AWeapon* InWeapon)
{	// проверим что не нулевые
	if (!IsValid(MultiplayerCharacter) || !IsValid(InWeapon)) return;
	// если в руках уже есть оружие, то выкинем его и поберем новое
	DropWeapon();
	
	// назначим переменную оружия
	Weapon = InWeapon;
	// назначим владельца, поменяем статус чтобы он больше не показывал надпись, отключим физику оружия и уберем показывания оружия
	Weapon->SetOwner(MultiplayerCharacter);
	Weapon->SetHUDAmmo_Public();
	Weapon->SetWeaponState(EWeaponState::EWC_Equipped);
	Weapon->ShowPickUpWidget(false);

	// получим значение с карты с CarriedAmmoMap и обновим HUD
	GetAndUpdateHudCarriedAmmo();
	// спавн звука поднимания оружия
	SpawnPickUpSound();
	// перезарядка оружия если пустое
	ReloadAmmoIfEmpty();
	
	// получим сокет из персонажа и проверим что он существует и прикрепим туда оружие
	AttackWeaponAtSocket(MultiplayerCharacter->RightHandSocketName);
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
		// прикрепляеи оружие на сокет в правой руке
		AttackWeaponAtSocket(MultiplayerCharacter->RightHandSocketName);
		// спавн звука поднимания оружия
		SpawnPickUpSound();
		// если после подбора пустая обойма перезарядим ее
		ReloadAmmoIfEmpty();
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

void UCombatComponent::SetIsAiming(bool bInAim)
{
	if (GetWeapon() == nullptr) return;
	
	bIsAiming = bInAim;
	// если целимся, то назначаем скорость другую
	if (MultiplayerCharacter)
	{
		MultiplayerCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
	// 16.1 при нажатии на прицел мы включим функцию добавление виджета и воспроизведение анимации виджета прицела
	if (MultiplayerCharacter && MultiplayerCharacter->IsLocallyControlled() && GetWeapon() && GetWeapon()->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		MultiplayerCharacter->ShowSniperScopeWidget(bIsAiming);
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
		
		// 18.9 при стрельбе у дробовика поставим в незанятый т.к можем прервать перезарядку
		if (Weapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
		{
			CombatState = ECombatState::ECT_Unoccupied;
		}
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
	ReloadAmmoIfEmpty();
	
}

void UCombatComponent::WeaponStartFire()
{
	// трассировка с середины экрана и направлена далее пока не столкнется
	TraceUnderCrosshairs(HitResult);
	HitLocation = HitResult.ImpactPoint;
	// установим коэфф увеличение прицела при стрельбе
	CrosshairShootingFactor = 0.75f;
		
	if (!HitResult.ImpactPoint.IsNearlyZero())
	{
		//HitTarget = HitResult.ImpactPoint;
		Fire(bIsFirePressed, HitLocation);
	}
}

bool UCombatComponent::CanFire()
{
	if (GetWeapon() && !GetWeapon()->IsEmpty() && bCanFire)
	{	// 18.8 если это дробовик то не будем смотреть на статус
		//чтобы сразу можно было выстрелить до пополнения всего боезапаса
		if (GetWeapon()->GetWeaponType() == EWeaponType::EWT_Shotgun)
		{
			return true;
		}
		else if (CombatState == ECombatState::ECT_Unoccupied)
		{
			return true;
		}
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
		// 18.9 у клиента при обновление патронов если этол дробовик сообщим что можно стрелять не до заряжаясь 
		if (Weapon && Weapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
		{
			bCanFire = true;
		}
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	// добавим значение в карту
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	// 7.1 добавим значение в карту
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRLAmmo);
	// 10.1 добавим запасные патроны для пистолета 
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	// 12.1 добавим запасные патроны для пистолета 
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SMG, StartingSMGAmmo);
	// 13.1 добавим запасные патроны для пистолета 
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	// 15.1 добавим запасные патроны для снайперской винтовки 
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperRifleAmmo);
	// 17.1 добавим запасные патроны для запускателя гранат 
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
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
		}
	}
}

void UCombatComponent::Reload()
{
	if ( GetWeapon() && CarriedAmmo > 0 && CombatState != ECombatState::ECT_Reloading && !GetWeapon()->IsFullAmmo() )
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


void AMultiplayerCharacter::PlayReloadMontage()
{
	// проверим что EquipWeapon и получаем AnimInstance
	UAnimInstance* AnimInstance =  GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{	// запускаем монтаж и в зависимости прицеливаемся мы или нет мы переходим к слоту
		AnimInstance->Montage_Play(ReloadMontage,1.f);
		FName SectionName;
		switch (GetWeapon()->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = "Rifle";
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = "RocketLauncher";
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = "Pistol";
			break;
		case EWeaponType::EWT_SMG:
			SectionName = "Pistol";
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = "Shotgun";
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = "SniperRifle";
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = "GrenadeLauncher";
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}
void UCombatComponent::OnRep_CombatState()
{
	if (!IsValid(MultiplayerCharacter) && !MultiplayerCharacter->IsLocallyControlled()) return;
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
	// 19.1 запуск анимации броска гранаты и самой гранаты
	case ECombatState::ECT_ThrowGrenade:
		HandleThrowGrenade();
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

void UCombatComponent::ShotgunReloading()
{	//18.0 сервером будем запускать обновление кол-во патронов на 1
	if (MultiplayerCharacter->HasAuthority())
	{
		UpdateShotgunAmmoValue();
	}
}

// 18.1 Сделаем функцию для перезарядки по 1 патрону в дробовик
void UCombatComponent::UpdateShotgunAmmoValue()
{	// 18.2 Проверим что есть запасные патроны и что он уже не полон
	if (!GetWeapon() && AmountToReload() == 0 && MultiplayerPlayerController == nullptr) return;
	
	int32 AmmoInMagazineNow = GetWeapon()->GetCurrentAmmo();
	// 18.3 вычтем запасные патроны
	if (CarriedAmmoMap.Contains(GetWeapon()->GetWeaponType()))
	{
		CarriedAmmoMap[GetWeapon()->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[GetWeapon()->GetWeaponType()];
	}
	// 18.4 обновим значение Carried у сервера, а клиента обновиться уже в OnRep_CarriedAmmo
	MultiplayerPlayerController = MultiplayerPlayerController == nullptr ?  Cast<AMultiplayerPlayerController>(MultiplayerCharacter->Controller) : MultiplayerPlayerController;
	if (MultiplayerPlayerController)
	{
		MultiplayerPlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	}
	// 18.5 и прибавим патроны в текущий магазин.
	GetWeapon()->SetCurrentAmmo(AmmoInMagazineNow + 1);
	// 18.6 обновим значение WeaponAmmo у сервера, а клиента обновиться уже в OnRep_WeaponAmmo
	if (MultiplayerPlayerController)
	{
		MultiplayerPlayerController->SetHUDWeaponAmmo(AmmoInMagazineNow + 1);
	}
	// 18.6.1 в конце перезарядки когда полон поставим режим закончилась перезарядка
	if (GetWeapon()->IsFullAmmo())
	{
		CombatState = ECombatState::ECT_Unoccupied;
		// 18.6.2 и у сервера сделаем окончании анимации, а у клиента она будет в AWeapon::OnRep_WeaponAmmo()
		if (MultiplayerCharacter->GetMesh())
		{
			MultiplayerCharacter->GetMesh()->GetAnimInstance()->Montage_JumpToSection("ShotgunReloadingFinished");
		}
	}
	// 18.6.3 это для того чтобы мы могли прервать анимацию и выстрелить
	bCanFire = true;
}

void UCombatComponent::ThrowGrenade()
{
	// 24.8 сделаем проверку на кол-во гранат
	if (GrenadesAmount <= 0) return;
	// 19.1 при нажатии проверяем что мы не заняты
	if (CombatState == ECombatState::ECT_Unoccupied)
	{	// 19.1.1 и включаем переключение статуса на бросок
		Server_ThrowGrenade();
	}
}

void UCombatComponent::Server_ThrowGrenade_Implementation()
{
	// 24.8 сделаем проверку на кол-во гранат
	if (GrenadesAmount <= 0) return;
	// 19.2 переключим статусы и бросим гранату для сервера
	CombatState = ECombatState::ECT_ThrowGrenade;
	// 19.2.1 запуск анимации, для клиента она будет в статусе
	HandleThrowGrenade();
}

void UCombatComponent::HandleThrowGrenade()
{	// 19.3 Запуститься анимации броска
	if (MultiplayerCharacter)
	{
		MultiplayerCharacter->PlayThrowGrenadeMontage();
		//20.1 перекладывания основного оружия в левую руку
		AttackWeaponAtSocket(MultiplayerCharacter->LeftHandSocketName);
		SetVisibilityToGrenade(true);
	}
}

void UCombatComponent::ThrowGrenadeFinished()
{	// 19.4 при окончании броска переключиться на статус не занятый
	if (MultiplayerCharacter && MultiplayerCharacter->HasAuthority())
	{
		CombatState = ECombatState::ECT_Unoccupied;
		//20.2 перекладывания основного оружия обратно в правую
		AttackWeaponAtSocket(MultiplayerCharacter->RightHandSocketName);
	}
}

void UCombatComponent::SetVisibilityToGrenade(bool bVisibilityGrenade)
{	//21.2 Изменим видимость гранаты в начале броска и в конце броска
	if (MultiplayerCharacter && MultiplayerCharacter->GetGrenadeStaticMesh())
	{
		MultiplayerCharacter->GetGrenadeStaticMesh()->SetVisibility(bVisibilityGrenade);
	}
}

void UCombatComponent::LaunchGrenade()
{	
	// 23.2 Запустим серверный спавн гранаты
	if (MultiplayerCharacter && MultiplayerCharacter->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitLocation);
	}
}

void UCombatComponent::ServerLaunchGrenade_Implementation(FVector_NetQuantize InHitLocation)
{
	//21.3 изменим видимость на False чтобы потом заспавнить гранату настоящую и у нас не было 2 гранаты
	if (MultiplayerCharacter && MultiplayerCharacter->GetGrenadeStaticMesh())
	{
		MultiplayerCharacter->GetGrenadeStaticMesh()->SetVisibility(false);
		//22.1 Начнем спавн гранаты
		UWorld* World = GetWorld();
		if (World && GrenadeClass)
		{	//22.2 получим направление от места попадания центра экрана до места спавна гранаты
			FVector ThrowDirection = InHitLocation - MultiplayerCharacter->GetGrenadeStaticMesh()->GetComponentLocation();
			DrawDebugLine(World,MultiplayerCharacter->GetGrenadeStaticMesh()->GetComponentLocation(), MultiplayerCharacter->GetGrenadeStaticMesh()->GetComponentLocation() + ThrowDirection,FColor::Red,false,5.f);
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = MultiplayerCharacter;
			SpawnParameters.Instigator = MultiplayerCharacter;
			// 22.3 Заспавнем гранату
			AProjectile* Grenade = World->SpawnActor<AProjectile>(GrenadeClass,
				MultiplayerCharacter->GetGrenadeStaticMesh()->GetComponentLocation(),
				ThrowDirection.Rotation(),
				SpawnParameters);
			// 24.9 Отнимем 1 гранату
			GrenadesAmount = FMath::Clamp(GrenadesAmount - 1, 0, GrenadesMaxAmount);
			// 24.10 Если сервер это клиент то обновим у него кол-во гранат
			if (MultiplayerCharacter->IsLocallyControlled())
			{
				UpdateHUDGrenadesAmount();
			}
		}		
	}
}

void UCombatComponent::OnRep_GrenadesAmount()
{	// 24.11 Обновим кол-во гранат у клиента при изменения числа
	UpdateHUDGrenadesAmount();
}

void UCombatComponent::UpdateHUDGrenadesAmount()
{
	// обновим значение GrenadesAmount у сервера, а клиента обновиться уже в OnRep_GrenadesAmount
	MultiplayerPlayerController = MultiplayerPlayerController == nullptr ?  Cast<AMultiplayerPlayerController>(MultiplayerCharacter->Controller) : MultiplayerPlayerController;
	if (MultiplayerPlayerController)
	{
		MultiplayerPlayerController->SetHUDGrenadesAmount(GrenadesAmount);
	}
}

















