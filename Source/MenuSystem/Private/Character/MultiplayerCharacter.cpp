// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/MultiplayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameMode/MultiplayerGameModeTrue.h"
#include "GameplayTypes/TurningInPlace.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "MultiplayerComponent/BuffComponent.h"
#include "MultiplayerComponent/CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"
#include "Player/MultiplayerPlayerController.h"
#include "Player/MultiplayerPlayerState.h"
#include "Weapon/Weapon.h"


AMultiplayerCharacter::AMultiplayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	// создадим стрелу
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	// будет отвечать ControlRotation
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->TargetArmLength = 500.0f;
	// создадим компонент камеры
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// не будет использовать ControlRotation, т.к стрела уже использует
	CameraBoom->bUsePawnControlRotation = false;

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->SetIsReplicated(true);

	BuffComponent = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	BuffComponent->SetIsReplicated(true);

	GetMesh()->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
	
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;

	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidgetComponent"));
	OverheadWidgetComponent->SetupAttachment(GetRootComponent());
	
	DissolveTimelineComponent = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
	//20.2 создадим из него компонент и прикрепи его к мешу и уберем коллизию
	GrenadeAttachmentComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeAttachmentComponent"));
	GrenadeAttachmentComponent->SetupAttachment(GetMesh(),"GrenadeSocket");
	GrenadeAttachmentComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrenadeAttachmentComponent->SetVisibility(false);
	
	TurningInPlace = TurningInPlace::ETIP_NotTurning;
	SetMinNetUpdateFrequency(33.f);
	SetNetUpdateFrequency(66.f);
	bReplicates = true;
}

void AMultiplayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (CombatComponent)
	{	// в компонент задаем что персонаж владеющий компонентом это мы
		CombatComponent->SetMultiplayerCharacter(this);
	}
	if (BuffComponent)
	{	// в компонент задаем что персонаж владеющий компонентом это мы
		BuffComponent->SetMultiplayerCharacter(this);
		// 25.13 Зададим стандартные значения скорости
		BuffComponent->SetInitialBaseSpeed(GetCharacterMovement()->MaxWalkSpeed,GetCharacterMovement()->MaxWalkSpeedCrouched);
		//26. Установим стандартную скорость прыжка
		BuffComponent->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}
}

void AMultiplayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// 5.1 немного почистим тик и сделаем тут отдельную функцию для вращения и наклона туловища
	RotateInPlace(DeltaTime);
	
	HideCameraIfCharacterClose();
	
	// функция будет пытаться обновить Score пока не получит PlayerState, потом перестанет
	PollInit();
}

void AMultiplayerCharacter::RotateInPlace(float DeltaTime)
{
	// 5.2 выключим управление когда GameState == Cooldown
	if (MultiplayerPlayerController && MultiplayerPlayerController->IsDisableGameplay()) return;
	
	if (GetLocalRole() > ROLE_SimulatedProxy )
	{// поворот туловища персонажа на месте
		AimOffset(DeltaTime);
	}
	else
	{// вычисления угла наклона камеры для изменения наклона персонажа
		CalculateAO_Pitch();
	}
	// если время с последнего ReplicatedMovement был больше 0.25 то обновим его
	if (DeltaTimeBetweenReplicate > 0.25f)
	{
		OnRep_ReplicatedMovement();
	}
	DeltaTimeBetweenReplicate += DeltaTime;
}

void AMultiplayerCharacter::PollInit()
{	// функция будет пытаться обновить счет пока не получит PlayerState
	if (MultiplayerPlayerState == nullptr)
	{
		MultiplayerPlayerState = GetPlayerState<AMultiplayerPlayerState>();
		if (MultiplayerPlayerState)
		{
			MultiplayerPlayerState->AddScorePoints(0.f);
			MultiplayerPlayerState->AddDefeatPoints(0);
		}
	}
	// 28.5 Сделаем некоторые функции HUD булевыми
	// и будем получать их статус активации, если false то будет перезапускать до победного
	if (IsLocallyControlled() && MultiplayerPlayerController)
	{
		if (bIsHealthInitialized == false)
		{
			bIsHealthInitialized = UpdateHUDHealth();
		}
		if (bIsShieldInitialized == false)
		{
			bIsShieldInitialized = UpdateHUDShield();
		}
		if (bIsAmmoInitialized == false)
		{
			bIsAmmoInitialized = UpdateHUDAmmo();
		}
		if (bIsCarriedAmmoInitialized == false)
		{
			bIsCarriedAmmoInitialized = UpdateHUDCarriedAmmo();
		}
	}
}

void AMultiplayerCharacter::Destroyed()
{
	// деактивируем эффект при уничтожения объекта
	if (ElimBotParticleComponent)
	{
		ElimBotParticleComponent->Deactivate();
	}
	 // 6.1 проверим что мы находимся в последней стадии матча иначе людям нужны будут оружия
	if (GetWeapon() && MultiplayerPlayerController && MultiplayerPlayerController->GetMatchState() == MatchState ::Cooldown)
	{
		GetWeapon()->Destroy();
	}
	
	Super::Destroyed();
}


void AMultiplayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	//31.5 заспавнем стандартное оружие игроку
	SpawnStandardWeapon();
	
	// получем контроллер и назначим стартовое макс хп и обычное хп
	UpdateHUDHealth();
	// привяжемся к делегату получения урона
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this,&AMultiplayerCharacter::ReceiveDamage);
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController)
	{
		FInputModeGameOnly ModeUI;
		PlayerController->SetInputMode(ModeUI);
	}
	
}

UCombatComponent* AMultiplayerCharacter::GetCombatComponent()
{
	return CombatComponent;
}

UBuffComponent* AMultiplayerCharacter::GetBuffComponent()
{
	return BuffComponent;
}

UCameraComponent* AMultiplayerCharacter::GetCameraComponent()
{
	return Camera;
}


bool AMultiplayerCharacter::UpdateHUDHealth()
{
	MultiplayerPlayerController = MultiplayerPlayerController == nullptr ?  Cast<AMultiplayerPlayerController>(Controller) : MultiplayerPlayerController;
	if (MultiplayerPlayerController)
	{	// обновим здоровье
		return MultiplayerPlayerController->SetHUDHealth(Health, MaxHealth);
	}
	else
	{
		return false;
	}
}

bool AMultiplayerCharacter::UpdateHUDShield()
{
	MultiplayerPlayerController = MultiplayerPlayerController == nullptr ?  Cast<AMultiplayerPlayerController>(Controller) : MultiplayerPlayerController;
	if (MultiplayerPlayerController)
	{	// обновим значение щита
		return MultiplayerPlayerController->SetHUDShield(Shield, MaxShield);
	}
	else { return false;}
}

bool AMultiplayerCharacter::UpdateHUDAmmo()
{
	MultiplayerPlayerController = MultiplayerPlayerController == nullptr ?  Cast<AMultiplayerPlayerController>(Controller) : MultiplayerPlayerController;
	if (MultiplayerPlayerController && GetWeapon())
	{	// обновим значение щита
		return MultiplayerPlayerController->SetHUDWeaponAmmo(GetWeapon()->GetCurrentAmmo());
	}
	else { return false;}
}

bool AMultiplayerCharacter::UpdateHUDCarriedAmmo()
{
	if (CombatComponent)
	{	// обновим значение щита
		return CombatComponent->GetAndUpdateHudCarriedAmmo();
	}
	else { return false;}
}

void AMultiplayerCharacter::AddHealPoint(float Amount)
{
	// 24.7 Восстановим здоровье но не больше максимального
	Health = FMath::Clamp(Health + Amount, 0.f, MaxHealth);
	// 24.8 Обновим значение у сервера у клиента она будет в OnRep_Health.
	UpdateHUDHealth();
}

void AMultiplayerCharacter::AddShieldPoint(float Amount)
{
	// 29. Восстановим щита но не больше максимального
	Shield = FMath::Clamp(Shield + Amount, 0.f, MaxHealth);
	// 29. Обновим значение у сервера у клиента она будет в OnRep_Health.
	UpdateHUDShield();
}

void AMultiplayerCharacter::HideCameraIfCharacterClose()
{
	if (IsLocallyControlled())
	{	// проверим дистанцию от камеры до игрока
		float DistanceFromCameraToPlayer = (GetActorLocation() - Camera->GetComponentLocation()).Size();
		if (DistanceFromCameraToPlayer < CameraThreshold)
		{	// выключаем видимость игрока и оружия
			GetMesh()->SetVisibility(false);
			if (CombatComponent->GetWeapon())
			{
				CombatComponent->GetWeapon()->GetWeaponMesh()->SetOwnerNoSee(true);
			}
		}
		else
		{	//включаем обратно
			GetMesh()->SetVisibility(true);
			if (CombatComponent->GetWeapon())
			{
				CombatComponent->GetWeapon()->GetWeaponMesh()->SetOwnerNoSee(false);
			}
		}
	}
}

void AMultiplayerCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{ 
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AMultiplayerCharacter,OverlappingWeapon,COND_OwnerOnly);
	DOREPLIFETIME(AMultiplayerCharacter,Health);
	DOREPLIFETIME(AMultiplayerCharacter,MaxHealth);
	DOREPLIFETIME(AMultiplayerCharacter,Shield);
	DOREPLIFETIME(AMultiplayerCharacter,MaxShield);
}

void AMultiplayerCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		SimProxiesTurn();
	}
	DeltaTimeBetweenReplicate = 0.f;
}


AWeapon* AMultiplayerCharacter::GetWeapon()
{
	if (CombatComponent)
	{	// в компонент задаем что персонаж владеющий компонентом это мы
		return CombatComponent->GetWeapon();
	}
	return nullptr;
}

void AMultiplayerCharacter::SetIsAiming(bool bIsAiming)
{
	if (CombatComponent)
	{
		CombatComponent->SetIsAiming(bIsAiming);
		if (!HasAuthority())
		{
			CombatComponent->ServerSetIsAiming(bIsAiming);
		}
	}
}

bool AMultiplayerCharacter::GetIsAiming()
{
	if (CombatComponent)
	{
		return CombatComponent->GetIsAiming();
	}
	return false;
}

void AMultiplayerCharacter::FireButtonPressed(bool IsPressed)
{
	if (CombatComponent)
	{
		bFireButtonPressed = IsPressed;
		CombatComponent->FireButtonPressed(IsPressed);
	}
}

FVector AMultiplayerCharacter::GetHitLocation() const
{
	if (CombatComponent)
	{
		return CombatComponent->HitLocation;
	}
	return FVector::ZeroVector;
}

/*
void AMultiplayerCharacter::FireMontagePlayAndSpawnBullet(bool IsFireButtonPressed, const FVector_NetQuantize& TargetPoint)
{
	if (CombatComponent && CombatComponent->GetWeapon() && IsFireButtonPressed && IsLocallyControlled())
	{	// получаем точку трассировки от центра экрана до пересечения объекта
		FVector& HitLocation = CombatComponent->GetFireTarget();
		// получаем вращение и локацию для пули
		FTransform SpawnTransform =CombatComponent->GetWeapon()->CalculateSpawnBulletTransform(HitLocation);
		// запускаем серверную функцию
		ServerFireMontagePlayAndSpawnBullet(TargetPoint);
	}
}

void AMultiplayerCharacter::ServerFireMontagePlayAndSpawnBullet_Implementation(const FVector_NetQuantize& TargetPoint)
{
	// обновляем вращение и локацию для спавна пули
	//CombatComponent->BulletSpawnTransform = BulletTransform;
	// далее проверяем что все ок и запускаем спавн пули
	if (CombatComponent->GetWeapon() && CombatComponent->BulletSpawnTransform.IsValid())
	{
		CombatComponent->GetWeapon()->OpenFire(TargetPoint);
	}
	// запускаем NetMulticast функцию для воспроизведения на всех клиентах
	MulticastFireMontagePlayAndSpawnBullet();
}
*/


void AMultiplayerCharacter::MulticastFireMontagePlay_Implementation()
{
	// проверим что EquipWeapon и получаем AnimInstance
	UAnimInstance* AnimInstance =  GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{	// запускаем монтаж и в зависимости прицеливаемся мы или нет мы переходим к слоту
		AnimInstance->Montage_Play(FireMontage,1.f);
		FName SlotName  = CombatComponent->GetIsAiming() ? "RiffleAim" : "RiffleHip";
		AnimInstance->Montage_JumpToSection(SlotName);
	}
}

void AMultiplayerCharacter::Elim()
{
	// запустим анимацию смерти
	MulticastElim();
	// запустим таймер по которому мы уничтожим игрока и запустим его новый спавн
	GetWorldTimerManager().SetTimer(EliminatedTimer,
		this,
		&AMultiplayerCharacter::ElimTimerFinisher,
		EliminatedDelay);
}

void AMultiplayerCharacter::ElimTimerFinisher()
{
	AMultiplayerGameModeTrue* MultiplayerGameMode = Cast<AMultiplayerGameModeTrue>(UGameplayStatics::GetGameMode(this));
	if (MultiplayerGameMode)
	{	// запустим отсоединение от контроллера и запуск нового спавна
		MultiplayerGameMode->RequestRespawn(this,Controller);
	}
}

void AMultiplayerCharacter::MulticastElim_Implementation()
{	// поставим true чтобы в AnimInstance отключить FABRIC и включить обычную стойку
	bIsEliminated = true;
	// и воспроизведем монтаж
	ElimMontagePlay();
	
	// запустим эффект dissolve
	StartDissolve();
	
	// проверим что контроллер действителен 
	if (MultiplayerPlayerController)
	{	// поменяем мод на UI only чтобы заблокировать Input
		FInputModeUIOnly ModeUI;
		//MultiplayerPlayerController->SetInputMode(ModeUI);
		MultiplayerPlayerController->SetHUDWeaponAmmo(0);
	}
	//16.1 При смерти в прицеле то уберем анимацию прицела
	if (IsLocallyControlled() && GetWeapon() && GetWeapon()->GetWeaponType() == EWeaponType::EWT_SniperRifle )
	{
		ShowSniperScopeWidget(false);
	}
	
	// отключим Передвижение и реагирование на нажатия
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// включим выпадение оружия и поменяем статус у оружия
	DropOrDestroyWeapon();
	
	/*
	*  Elim бот эффект и звук
	*/
	
	FVector ElimParticleSpawnPoint = GetActorLocation();
	ElimParticleSpawnPoint += FVector(0, 0, 200);
	ElimBotParticleComponent = UGameplayStatics::SpawnEmitterAtLocation(this,ElimBotParticle,ElimParticleSpawnPoint,GetActorRotation());
	UGameplayStatics::SpawnSoundAtLocation(this, ElimSound,ElimParticleSpawnPoint,GetActorRotation());
	
}

void AMultiplayerCharacter::DropOrDestroyWeapon()
{	
	if (GetCombatComponent())
	{
		if (GetWeapon() )
		{
			GetWeapon()->Dropped();
		}
		if (GetCombatComponent()->GetSecondaryWeapon())
		{
			GetCombatComponent()->GetSecondaryWeapon()->Dropped();
		}
	}
}

void AMultiplayerCharacter::ElimMontagePlay()
{
	// Запустим монтаж смерти при хп = 0
	UAnimInstance* AnimInstance =  GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{	// запускаем монтаж 
		AnimInstance->Montage_Play(ElimMontage,1.f);
	}
}

void AMultiplayerCharacter::StartDissolve()
{
	// привяжемся к делегату обнолвение TimelIne
	DissolveTimelineEvent.BindDynamic(this,&AMultiplayerCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimelineComponent)
	{	// создадим динамич материал из обычного материала
		DissolveDynamicMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		// назначим материал нашему мешу
		GetMesh()->SetMaterial(0, DissolveDynamicMaterialInstance);

		// добавим наш делегат в пул делегатов в Timeline
		DissolveTimelineComponent->AddInterpFloat(DissolveCurve,DissolveTimelineEvent);
		// стартанем Timeline
		DissolveTimelineComponent->PlayFromStart();
	}
}

void AMultiplayerCharacter::UpdateDissolveMaterial(float Value)
{
	if (DissolveDynamicMaterialInstance)
	{	// будем обновлять параметр Dissolve каждый момент Timeline
		DissolveDynamicMaterialInstance->SetScalarParameterValue("Dissolve", Value);
	}
}

void AMultiplayerCharacter::SpawnStandardWeapon()
{
	// получем GameMode и World для спавна, Gamemode должен быть этот а нет тот что в лобби
	AMultiplayerGameModeTrue* MultiplayerGameMode = Cast<AMultiplayerGameModeTrue>(UGameplayStatics::GetGameMode(this));
	UWorld* World = GetWorld();
	if (MultiplayerGameMode && World && StandartWeaponClass)
	{	// заспавнем и запомним указатель
		StandardWeapon = World->SpawnActor<AWeapon>(StandartWeaponClass);
		
		if (StandardWeapon && CombatComponent)
		{	// экипируем его
			CombatComponent->EquipWeapon(StandardWeapon);
			// отметим что это стандартное оружие и при выбрасывае оно учитожиться
			StandardWeapon->SetbStandardWeapon(true);
		}
	}
}


void AMultiplayerCharacter::MulticastHitMontagePlay_Implementation()
{
	HitReactPlayMontage();
}

void AMultiplayerCharacter::HitReactPlayMontage()
{
	if (HitMontage && CombatComponent->GetWeapon())
	{
		UAnimInstance* AnimInstance =  GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{	// запускаем монтаж и в зависимости прицеливаемся мы или нет мы переходим к слоту
			AnimInstance->Montage_Play(HitMontage,1.f);
			AnimInstance->Montage_JumpToSection("FromFront");
		}
	}
}

void AMultiplayerCharacter::AimOffset(float DeltaTime)
{
	//
	// для AimOffset
	//
	if (!CombatComponent || !CombatComponent->GetWeapon()) return;

	float Speed = CalculateSpeed();
	
	// проверяем если персонаж не двигается, то будет двигать корпусом в ABP анимации
	if (Speed == 0.f  && !GetCharacterMovement()->IsFalling() && IsLocallyControlled())
	{
		bRotateRootBone = true;
		FRotator AimRotation = FRotator(0.f,GetBaseAimRotation().Yaw,0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(AimRotation, StartingCharacterRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		bUseControllerRotationYaw = true;
		// пока мы не поворачиваем мы запоминаем AO_Yaw чтобы потом плавно повернуть
		if (TurningInPlace == TurningInPlace::ETIP_NotTurning)
		{
			Interp_AO_Yaw = AO_Yaw;
		}
		// поворот ног на месте при превышении AO_Yaw 90 градусов
		TurnInPlace(DeltaTime);
	}
	else 
	{ // если двигается, то получим GetBaseAimRotation, чтобы запомнить последнее положение при движение 
		StartingCharacterRotation =  FRotator(0.f,GetBaseAimRotation().Yaw,0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = TurningInPlace::ETIP_NotTurning;
		bRotateRootBone = false;
	}
	CalculateAO_Pitch();
}

float AMultiplayerCharacter::CalculateSpeed()
{
	FVector Velocity  = GetCharacterMovement()->Velocity;
	Velocity.Z = 0;
	return Velocity.Size();
}

void AMultiplayerCharacter::CalculateAO_Pitch()
{
	// наклон для aimOffset
	AO_Pitch = GetBaseAimRotation().Pitch;
	// для клиента, т.к там сжимается переменная для репликации то из отрицательного она превращается в положительное и выходит не свопадение
	// поэтому сделаем 2 вектора и соотнося что 270 = -90 будет назначать значение для AO_Pitch т.е если AO_Pitch = 350 то станет -10
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270,360) to [-90,0)
		FVector2D AngleOnTheClient (270.f,360.f);
		FVector2D CurrentAngleSector (-90.f,0);
		AO_Pitch = FMath::GetMappedRangeValueClamped(AngleOnTheClient,CurrentAngleSector,AO_Pitch);
	}
}

void AMultiplayerCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{ // если больше 90 то надо повернуть направо ногами
		TurningInPlace = TurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{// если меньше -90 то надо повернуть направо ногами
		TurningInPlace = TurningInPlace::ETIP_Left;
	}
	// когда мы поворачиваем куда либо срабатывает условие
	if (TurningInPlace != TurningInPlace::ETIP_NotTurning)
	{	// мы медленно снижаем Interp_AO_Yaw до 0 и присваиваем его в AO_Yaw. В ABP_AimInstance у нас RotateRootBone
		// он поворачивал к -AO_Yaw. Когда он будет снижаться до нуля он будет он будет поворачивать персонажа пока не будет < 15.f
		Interp_AO_Yaw = FMath::FInterpTo(Interp_AO_Yaw,0,DeltaTime,4.f);
		AO_Yaw = Interp_AO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{	// когда < 15.f мы поменяем enum для анимации и условия нашего и обновим StartingCharacterRotation, чтобы AimOffset() работал адекватно
			TurningInPlace = TurningInPlace::ETIP_NotTurning;
			StartingCharacterRotation =  FRotator(0.f,GetBaseAimRotation().Yaw,0.f);
		}
	}
	// для проверки
	UE_LOG(LogTemp,Warning,TEXT("AO_Yaw: %f"),AO_Yaw);
}

void AMultiplayerCharacter::SimProxiesTurn()
{
	if (CombatComponent == nullptr || !CombatComponent->GetWeapon()) return;
	// выключим вращение корпуса у Simulated proxy
	bRotateRootBone = false;
	// теперь если мы начинаем двигать то выкключим анимации поворота
	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = TurningInPlace::ETIP_NotTurning;
		return;
	}
	
	// будем сравнивать вращение между последним кадром 
	RotationActorLastFrame = RotationActorCurrentFrame;
	RotationActorCurrentFrame = GetActorRotation();
	DeltaRotationBetweenFrames = UKismetMathLibrary::NormalizedDeltaRotator(RotationActorCurrentFrame,RotationActorLastFrame).Yaw;
	// и если оно больше чем 0.5 то будем менять статус и потом в AnimInstance он сам будет переходить по статусу на другую анимацию 
	if (FMath::Abs(DeltaRotationBetweenFrames) > RotationThreshold)
	{
		if (DeltaRotationBetweenFrames > RotationThreshold)
		{
			TurningInPlace = TurningInPlace::ETIP_Right;
		}
		else if (DeltaRotationBetweenFrames < RotationThreshold)
		{
			TurningInPlace = TurningInPlace::ETIP_Left;
		}
	}
	else
	{
		TurningInPlace = TurningInPlace::ETIP_NotTurning;
	}
	UE_LOG(LogTemp,Warning,TEXT("DeltaRotationBetweenFrames: %f"),DeltaRotationBetweenFrames);
}

void AMultiplayerCharacter::OnRep_Health(float OldValue)
{
	if (OldValue > Health)
	{
		// запустим монтаж получения урона
		MulticastHitMontagePlay();
	}
	// обновим здоровье
	UpdateHUDHealth();
}

void AMultiplayerCharacter::OnRep_MaxHealth()
{
}

void AMultiplayerCharacter::OnRep_Shield(float OldValue)
{
	if (OldValue > Shield)
	{
		// запустим монтаж получения урона
		MulticastHitMontagePlay();
	}
	UpdateHUDShield();
}

void AMultiplayerCharacter::OnRep_MaxShield()
{
	
}

void AMultiplayerCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
                                          class AController* InstigatedBy, AActor* DamageCauser)
{
	// 28.1 Сделаем временную переменную урона и будем менять его если есть щит
	float DamageToHealth  = Damage;
	if (Shield > 0.f)
	{	// если щит больше урона
		if (Shield > DamageToHealth)
		{
			Shield = FMath::Clamp(Shield - DamageToHealth, 0.f, MaxShield);
			DamageToHealth = 0.f;
		}
		else // если щит меньше урона
		{
			Shield = 0.f;
			DamageToHealth -= Shield;
		}
		UpdateHUDShield();
	}
	//28.2 если щит меньше урона или его нет то наносится урон по здоровье
	Health = FMath::Clamp(Health-DamageToHealth,0.f,MaxHealth);
	
	UpdateHUDHealth();
	MulticastHitMontagePlay();
	
	// когда буедт 0 хп, мы вызовем в GameMode PlayerEliminated
	if (Health == 0.f)
	{
		AMultiplayerGameModeTrue* MultiplayerGameMode = Cast<AMultiplayerGameModeTrue>(UGameplayStatics::GetGameMode(this));
		if (MultiplayerGameMode)
		{ // когда буедт 0 хп, мы вызовем в GameMode PlayerEliminated
			MultiplayerPlayerController = MultiplayerPlayerController == nullptr ?  Cast<AMultiplayerPlayerController>(Controller) : MultiplayerPlayerController;
			AMultiplayerPlayerController* EnemyController = Cast<AMultiplayerPlayerController>(InstigatedBy);
			if (!EnemyController) return;
			MultiplayerGameMode->PlayerEliminated(this,MultiplayerPlayerController,EnemyController);
		}
	}
}


void AMultiplayerCharacter::OnRep_OverlappingWeapon(AWeapon* OldValue)
{
	// если старое значение имело указатель ,то уберем видимость виджета
	if (OldValue)
	{
		OldValue->ShowPickUpWidget(false);
	}
	// если новое значение валидный указатель, то показываем виджет
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickUpWidget(true);
	}
}


void AMultiplayerCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// если уже есть указатель то уберем видимость у виджета
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickUpWidget(false);
	}
	// назначаем weapon потом запуститься репликация
	OverlappingWeapon = Weapon;
	// проверяем что этот персонал либо клиент который владеет персонажем, либо сервер играющий за персонажа
	if (IsLocallyControlled())
	{	
		if (OverlappingWeapon)
		{	// включаем видимость виджета
			OverlappingWeapon->ShowPickUpWidget(true);
		}
	}
}

void AMultiplayerCharacter::EquipWeapon()
{
	if (OverlappingWeapon && CombatComponent)
	{	// если сервер, то выполняем просто EquipWeapon
		if (HasAuthority())
		{
			CombatComponent->EquipWeapon(OverlappingWeapon);
		}
		else
		{// если клиент, то запускаем специальную серверную функцию
			ServerEquipWeapon();
		}
	}
	else if (CombatComponent && CombatComponent->GetSecondaryWeapon())
	{
		CombatComponent->SwapWeapon();
	}
}

void AMultiplayerCharacter::ServerEquipWeapon_Implementation()
{
	CombatComponent->EquipWeapon(OverlappingWeapon);
}

// 5.3 получение булевой выключения управление и вращения когда GameState == Cooldown
bool AMultiplayerCharacter::IsDisabledGameplay()
{
	if (MultiplayerPlayerController)
	{
		return MultiplayerPlayerController->IsDisableGameplay();
	}
	return false;
}


void AMultiplayerCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance =  GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{	// запускаем монтаж броска гранаты
		AnimInstance->Montage_Play(GrenadeThrowMontage,1.f);
	}
}